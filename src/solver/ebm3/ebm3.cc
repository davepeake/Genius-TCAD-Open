/********************************************************************************/
/*     888888    888888888   88     888  88888   888      888    88888888       */
/*   8       8   8           8 8     8     8      8        8    8               */
/*  8            8           8  8    8     8      8        8    8               */
/*  8            888888888   8   8   8     8      8        8     8888888        */
/*  8      8888  8           8    8  8     8      8        8            8       */
/*   8       8   8           8     8 8     8      8        8            8       */
/*     888888    888888888  888     88   88888     88888888     88888888        */
/*                                                                              */
/*       A Three-Dimensional General Purpose Semiconductor Simulator.           */
/*                                                                              */
/*                                                                              */
/*  Copyright (C) 2007-2008                                                     */
/*  Cogenda Pte Ltd                                                             */
/*                                                                              */
/*  Please contact Cogenda Pte Ltd for license information                      */
/*                                                                              */
/*  Author: Gong Ding   gdiso@ustc.edu                                          */
/*                                                                              */
/********************************************************************************/



#include "ebm3/ebm3.h"
#include "parallel.h"
#include "petsc_utils.h"


using PhysicalUnit::kb;
using PhysicalUnit::e;
using PhysicalUnit::cm;
using PhysicalUnit::K;



/*------------------------------------------------------------------
 * create nonlinear solver contex and adjust some parameters
 */
int EBM3Solver::create_solver()
{

  MESSAGE<< '\n' << "Energy Balance Solver init..." << std::endl;
  RECORD();

  // set ac variables for each region
  set_variables();

  return DDMSolverBase::create_solver();

}

/*------------------------------------------------------------------
 * prepare solution and aux variables used by this solver
 */
int EBM3Solver::set_variables()
{
  for ( unsigned int n=0; n<_system.n_regions(); n++ )
  {
    SimulationRegion * region = _system.region ( n );
    if ( region->type()  == SemiconductorRegion)
    {
      region->add_variable("elec_temperature", POINT_CENTER);
      region->add_variable("hole_temperature", POINT_CENTER);
      region->add_variable("elec_temperature.last", POINT_CENTER);
      region->add_variable("hole_temperature.last", POINT_CENTER);
    }
  }

  return 0;
}


/*------------------------------------------------------------------
 * set initial value to solution vector and scaling vector
 */
int EBM3Solver::pre_solve_process(bool load_solution)
{
  if(load_solution)
  {
    // for all the regions
    for(unsigned int n=0; n<_system.n_regions(); n++)
    {
      SimulationRegion * region = _system.region(n);
      region->EBM3_Fill_Value(x, L);
    }

    // for all the bcs
    for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
    {
      BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
      bc->EBM3_Fill_Value(x, L);
    }

    VecAssemblyBegin(x);
    VecAssemblyBegin(L);

    VecAssemblyEnd(x);
    VecAssemblyEnd(L);
  }

  return DDMSolverBase::pre_solve_process(load_solution);
}





/*------------------------------------------------------------------
 * wrap to each solve implimention
 */
int EBM3Solver::solve()
{

  START_LOG("EBM3Solver_SNES()", "EBM3Solver");

  switch( SolverSpecify::Type )
  {
      case SolverSpecify::EQUILIBRIUM :
      solve_equ(); break;

      case SolverSpecify::STEADYSTATE:
      solve_steadystate(); break;

      case SolverSpecify::DCSWEEP:
      solve_dcsweep(); break;

      case SolverSpecify::TRANSIENT:
      solve_transient();break;

      case SolverSpecify::TRACE:
      solve_iv_trace();break;

      default:
      MESSAGE<< '\n' << "EBM3Solver: Unsupported solve type."; RECORD();
      genius_error();
      break;
  }

  STOP_LOG("EBM3Solver_SNES()", "EBM3Solver");

  return 0;
}




/*------------------------------------------------------------------
 * restore the solution to each region
 */
int EBM3Solver::post_solve_process()
{

  VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);

  PetscScalar *lxx;
  VecGetArray(lx, &lxx);

  //search for all the regions
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Update_Solution(lxx);
  }

  // update bcs
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Update_Solution(lxx);
  }

  VecRestoreArray(lx, &lxx);

  return DDMSolverBase::post_solve_process();
}


/*------------------------------------------------------------------
 * write the (intermediate) solution to each region
 */
void EBM3Solver::flush_system()
{
  VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);

  PetscScalar *lxx;
  VecGetArray(lx, &lxx);

  //search for all the regions
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Update_Solution(lxx);
  }

  VecRestoreArray(lx, &lxx);
}



/*------------------------------------------------------------------
 * load previous state into solution vector
 */
int EBM3Solver::diverged_recovery()
{
  // for all the regions
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Fill_Value(x, L);
  }

  // for all the bcs
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Fill_Value(x, L);
  }

  VecAssemblyBegin(x);
  VecAssemblyBegin(L);

  VecAssemblyEnd(x);
  VecAssemblyEnd(L);

  return 0;
}


/*------------------------------------------------------------------
 * Potential Newton Damping
 */
void EBM3Solver::potential_damping(Vec x, Vec y, Vec w, PetscBool *changed_y, PetscBool *changed_w)
{

  PetscScalar    *xx;
  PetscScalar    *yy;
  PetscScalar    *ww;

  VecGetArray(x, &xx);  // previous iterate value
  VecGetArray(y, &yy);  // new search direction and length
  VecGetArray(w, &ww);  // current candidate iterate

  PetscScalar dV_max = 0.0; // the max changes of psi
  const PetscScalar onePerCMC = 1.0*std::pow(cm,-3);
  const PetscScalar T_external = this->get_system().T_external();
  // we should find dV_max;
  // first, we find in local
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    // only consider semiconductor region
    const SimulationRegion * region = _system.region(n);
    if( region->type() != SemiconductorRegion ) continue;

    unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
    unsigned int node_n_offset   = region->ebm_variable_offset(ELECTRON);
    unsigned int node_p_offset   = region->ebm_variable_offset(HOLE);
    unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);
    unsigned int node_Tn_offset  = region->ebm_variable_offset(E_TEMP);
    unsigned int node_Tp_offset  = region->ebm_variable_offset(H_TEMP);

    SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
    SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
    for(; it!=it_end; ++it)
    {
      const FVM_Node * fvm_node = *it;
      unsigned int local_offset = fvm_node->local_offset();

      if(  std::abs( yy[local_offset + node_psi_offset] ) > dV_max)
        dV_max = std::abs(yy[local_offset + node_psi_offset]);

      //prevent negative carrier density
      if ( ww[local_offset+node_n_offset] < onePerCMC )
        ww[local_offset+node_n_offset] = onePerCMC;
      if ( ww[local_offset+node_p_offset] < onePerCMC )
        ww[local_offset+node_p_offset] = onePerCMC;

      // limit lattice temperature to env temperature - 50k
      if(region->get_advanced_model()->enable_Tl())
      {
        if ( ww[local_offset+node_Tl_offset] < T_external - 50*K )
          ww[local_offset+node_Tl_offset] = T_external - 50*K;
      }
      // electrode temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tn())
      {
        PetscScalar n0=xx[local_offset+node_n_offset];
        PetscScalar n1=ww[local_offset+node_n_offset];
        PetscScalar T0=xx[local_offset+node_Tn_offset]/n0;
        PetscScalar T1=T0*(1-std::min(n1/n0,2.0)) + ww[local_offset+node_Tn_offset]/n0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        ww[local_offset+node_Tn_offset] = T1*n1;
      }
      // hole temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tp())
      {
        PetscScalar p0=xx[local_offset+node_p_offset];
        PetscScalar p1=ww[local_offset+node_p_offset];
        PetscScalar T0=xx[local_offset+node_Tp_offset]/p0;
        PetscScalar T1=T0*(1-std::min(p1/p0,2.0)) + ww[local_offset+node_Tp_offset]/p0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        ww[local_offset+node_Tp_offset] = T1*p1;
      }
    }
  }

  // for parallel situation, we should find the dv_max in global.
  Parallel::max( dV_max );

  if( dV_max > 1e-6 )
  {
    // compute logarithmic potential damping factor f;
    PetscScalar Vut = kb*T_external/e * SolverSpecify::potential_update;
    PetscScalar f = log(1+dV_max/Vut)/(dV_max/Vut);

    // do newton damping here
    for(unsigned int n=0; n<_system.n_regions(); n++)
    {
      // only consider semiconductor region
      const SimulationRegion * region = _system.region(n);
      if( region->type() != SemiconductorRegion ) continue;

      unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);

      SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
      SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
      for(; it!=it_end; ++it)
      {
        const FVM_Node * fvm_node = *it;
        unsigned int local_offset = fvm_node->local_offset();

        ww[local_offset + node_psi_offset] = xx[local_offset + node_psi_offset] - f*yy[local_offset + node_psi_offset];
      }
    }

    // only the last processor do this
    if(Genius::is_last_processor())
    {
      for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
      {
        BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
        unsigned int array_offset = bc->array_offset();
        if(array_offset != invalid_uint)
          ww[array_offset] = xx[array_offset] - f*yy[array_offset];
      }
    }
  }

  VecRestoreArray(x, &xx);
  VecRestoreArray(y, &yy);
  VecRestoreArray(w, &ww);

  *changed_y = PETSC_FALSE;
  *changed_w = PETSC_TRUE;

  return;
}



/*------------------------------------------------------------------
 * Bank-Rose Newton Damping
 */
void EBM3Solver::bank_rose_damping(Vec , Vec , Vec , PetscBool *changed_y, PetscBool *changed_w)
{
  *changed_y = PETSC_FALSE;
  *changed_w = PETSC_FALSE;

  return;
}



/*------------------------------------------------------------------
 * positive density Newton Damping
 */
void EBM3Solver::positive_density_damping(Vec x, Vec y, Vec w, PetscBool *changed_y, PetscBool *changed_w)
{

  PetscScalar    *xx;
  PetscScalar    *yy;
  PetscScalar    *ww;

  VecGetArray(x, &xx);  // previous iterate value
  VecGetArray(y, &yy);  // new search direction and length
  VecGetArray(w, &ww);  // current candidate iterate

  int changed_flag=0;
  const PetscScalar onePerCMC = 1.0*std::pow(cm,-3);
  const PetscScalar T_external = this->get_system().T_external();

  // do newton damping here
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    // only consider semiconductor region
    const SimulationRegion * region = _system.region(n);
    if( region->type() != SemiconductorRegion ) continue;

    unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
    unsigned int node_n_offset   = region->ebm_variable_offset(ELECTRON);
    unsigned int node_p_offset   = region->ebm_variable_offset(HOLE);
    unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);
    unsigned int node_Tn_offset  = region->ebm_variable_offset(E_TEMP);
    unsigned int node_Tp_offset  = region->ebm_variable_offset(H_TEMP);

    SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
    SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
    for(; it!=it_end; ++it)
    {
      const FVM_Node * fvm_node = *it;
      unsigned int local_offset = fvm_node->local_offset();

      // psi update should not large than 1V
      if ( fabs(yy[local_offset + node_psi_offset]) > 1.0 )
      {
        ww[local_offset + node_psi_offset] = xx[local_offset + node_psi_offset] - std::sign(yy[local_offset + node_psi_offset ])*1.0;
      }

      //prevent negative carrier density
      if ( ww[local_offset+1] < onePerCMC )
      {
        ww[local_offset + node_n_offset] = onePerCMC;
      }

      if ( ww[local_offset + node_p_offset] < onePerCMC )
      {
        ww[local_offset + node_p_offset] = onePerCMC;
      }

      // limit lattice temperature to env temperature - 50k
      if(region->get_advanced_model()->enable_Tl())
      {
        if ( ww[local_offset+node_Tl_offset] < T_external - 50*K )
        { ww[local_offset+node_Tl_offset] = T_external - 50*K;  changed_flag=1;}
      }
      // electrode temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tn())
      {
        PetscScalar n0=xx[local_offset+node_n_offset];
        PetscScalar n1=ww[local_offset+node_n_offset];
        PetscScalar T0=xx[local_offset+node_Tn_offset]/n0;
        PetscScalar T1=T0*(1-std::min(n1/n0,2.0)) + ww[local_offset+node_Tn_offset]/n0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        ww[local_offset+node_Tn_offset] = T1*n1;
      }
      // hole temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tp())
      {
        PetscScalar p0=xx[local_offset+node_p_offset];
        PetscScalar p1=ww[local_offset+node_p_offset];
        PetscScalar T0=xx[local_offset+node_Tp_offset]/p0;
        PetscScalar T1=T0*(1-std::min(p1/p0,2.0)) + ww[local_offset+node_Tp_offset]/p0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        ww[local_offset+node_Tp_offset] = T1*p1;
      }

    }
  }

  VecRestoreArray(x, &xx);
  VecRestoreArray(y, &yy);
  VecRestoreArray(w, &ww);

  *changed_y = PETSC_FALSE;
  *changed_w = PETSC_TRUE;

  return;
}



void EBM3Solver::projection_positive_density_check(Vec x, Vec xo)
{
  PetscScalar    *xx;
  PetscScalar    *oo;

  VecGetArray(x, &xx);
  VecGetArray(xo, &oo);

  const PetscScalar onePerCMC = 1.0*std::pow(cm,-3);
  const PetscScalar T_external = this->get_system().T_external();

  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    // only consider semiconductor region
    const SimulationRegion * region = _system.region(n);
    if( region->type() != SemiconductorRegion ) continue;

    unsigned int node_n_offset   = region->ebm_variable_offset(ELECTRON);
    unsigned int node_p_offset   = region->ebm_variable_offset(HOLE);
    unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);
    unsigned int node_Tn_offset  = region->ebm_variable_offset(E_TEMP);
    unsigned int node_Tp_offset  = region->ebm_variable_offset(H_TEMP);

    SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
    SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
    for(; it!=it_end; ++it)
    {
      const FVM_Node * fvm_node = *it;
      unsigned int local_offset = fvm_node->local_offset();

      //prevent negative carrier density
      if ( xx[local_offset+node_n_offset] < onePerCMC )
        xx[local_offset+node_n_offset]= onePerCMC;

      if ( xx[local_offset+node_p_offset] < onePerCMC )
        xx[local_offset+node_p_offset]= onePerCMC;

      // limit lattice temperature to env temperature - 50k
      if(region->get_advanced_model()->enable_Tl())
      {
        if ( xx[local_offset+node_Tl_offset] < T_external - 50*K )
          xx[local_offset+node_Tl_offset] = T_external - 50*K;
      }

      // electrode temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tn())
      {
        PetscScalar n0=oo[local_offset+node_n_offset];
        PetscScalar n1=xx[local_offset+node_n_offset];
        PetscScalar T0=oo[local_offset+node_Tn_offset]/n0;
        PetscScalar T1=T0*(1-std::min(n1/n0,2.0)) + xx[local_offset+node_Tn_offset]/n0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        xx[local_offset+node_Tn_offset] = T1*n1;
      }

      // hole temperature should not below 90% of lattice temperature
      if(region->get_advanced_model()->enable_Tp())
      {
        PetscScalar p0=oo[local_offset+node_p_offset];
        PetscScalar p1=xx[local_offset+node_p_offset];
        PetscScalar T0=oo[local_offset+node_Tp_offset]/p0;
        PetscScalar T1=T0*(1-std::min(p1/p0,2.0)) + xx[local_offset+node_Tp_offset]/p0;
        if (T1 < 0.9*T_external)
          T1 = 0.9*T_external;
        xx[local_offset+node_Tp_offset] = T1*p1;
      }
    }
  }

  VecRestoreArray(x,&xx);
  VecRestoreArray(xo,&oo);
}



/*------------------------------------------------------------------
 * evaluate local truncation error
 */
PetscScalar EBM3Solver::LTE_norm()
{

  // time steps
  PetscScalar hn  = SolverSpecify::dt;
  PetscScalar hn1 = SolverSpecify::dt_last;
  PetscScalar hn2 = SolverSpecify::dt_last_last;

  // relative error
  PetscScalar eps_r = SolverSpecify::TS_rtol;
  // abs error
  PetscScalar eps_a = SolverSpecify::TS_atol;

  VecZeroEntries(xp);
  VecZeroEntries(LTE);

  // get the predict solution vector and LTE vector
  if(SolverSpecify::TS_type == SolverSpecify::BDF1)
  {
    VecAXPY(xp, 1+hn/hn1, x_n);
    VecAXPY(xp, -hn/hn1, x_n1);
    VecAXPY(LTE, hn/(hn+hn1), x);
    VecAXPY(LTE, -hn/(hn+hn1), xp);
  }
  else if(SolverSpecify::TS_type == SolverSpecify::BDF2)
  {
    PetscScalar cn  = 1+hn*(hn+2*hn1+hn2)/(hn1*(hn1+hn2));
    PetscScalar cn1 = -hn*(hn+hn1+hn2)/(hn1*hn2);
    PetscScalar cn2 = hn*(hn+hn1)/(hn2*(hn1+hn2));

    VecAXPY(xp, cn,  x_n);
    VecAXPY(xp, cn1, x_n1);
    VecAXPY(xp, cn2, x_n2);
    VecAXPY(LTE, hn/(hn+hn1+hn2),  x);
    VecAXPY(LTE, -hn/(hn+hn1+hn2), xp);
  }

  int N=0;
  PetscScalar r;


  // with LTE vector and relative & abs error, we get the error estimate here
  PetscScalar    *xx, *ll;
  VecGetArray(x, &xx);
  VecGetArray(LTE, &ll);

  // error estimate for each region
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    const SimulationRegion * region = _system.region(n);
    switch ( region->type() )
    {
        case SemiconductorRegion :
        {
          unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
          unsigned int node_n_offset   = region->ebm_variable_offset(ELECTRON);
          unsigned int node_p_offset   = region->ebm_variable_offset(HOLE);
          unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);
          unsigned int node_Tn_offset  = region->ebm_variable_offset(E_TEMP);
          unsigned int node_Tp_offset  = region->ebm_variable_offset(H_TEMP);

          SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
          SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
          for(; it!=it_end; ++it)
          {
            const FVM_Node * fvm_node = *it;
            unsigned int local_offset = fvm_node->local_offset();

            ll[local_offset+node_psi_offset] = 0;
            ll[local_offset+node_n_offset] = ll[local_offset+node_n_offset]/(eps_r*xx[local_offset+node_n_offset]+eps_a);
            ll[local_offset+node_p_offset] = ll[local_offset+node_p_offset]/(eps_r*xx[local_offset+node_p_offset]+eps_a);

            if(region->get_advanced_model()->enable_Tl())
              ll[local_offset+node_Tl_offset] = ll[local_offset+node_Tl_offset]/(eps_r*xx[local_offset+node_Tl_offset]+eps_a); // temperature

            if(region->get_advanced_model()->enable_Tn())
              ll[local_offset+node_Tn_offset] = ll[local_offset+node_Tn_offset]/(eps_r*xx[local_offset+node_Tn_offset]+eps_a); // temperature

            if(region->get_advanced_model()->enable_Tp())
              ll[local_offset+node_Tp_offset] = ll[local_offset+node_Tp_offset]/(eps_r*xx[local_offset+node_Tp_offset]+eps_a); // temperature
          }

          N += (region->ebm_n_variables()-1)*region->n_on_processor_node();

          break;
        }
        case InsulatorRegion :
        case ElectrodeRegion :
        case MetalRegion     :
        {
          unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
          unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);

          SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
          SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
          for(; it!=it_end; ++it)
          {
            const FVM_Node * fvm_node = *it;
            unsigned int local_offset = fvm_node->local_offset();

            ll[local_offset+node_psi_offset] = 0;

            if(region->get_advanced_model()->enable_Tl())
              ll[local_offset+node_Tl_offset] = ll[local_offset+node_Tl_offset]/(eps_r*xx[local_offset+node_Tl_offset]+eps_a); // temperature
          }

          N += (region->ebm_n_variables()-1)*region->n_on_processor_node();

          break;
        }
        case VacuumRegion:
        break;
        default: genius_error();
    }
  }


  // error estimate for each bc
  if( Genius::processor_id() == Genius::n_processors()-1 )
    for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
    {
      BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
      unsigned int array_offset = bc->array_offset();
      if( array_offset != invalid_uint )
        ll[array_offset] = 0;
    }

  VecRestoreArray(x, &xx);
  VecRestoreArray(LTE, &ll);

  VecNorm(LTE, NORM_2, &r);

  //for parallel situation, we should sum N of all the processor
  Parallel::sum( N );

  if( N>0 )
  {
    r /= sqrt(static_cast<double>(N));
    return r;
  }

  return 1.0;

}



void EBM3Solver::error_norm()
{
  PetscScalar    *xx;
  PetscScalar    *ff;

  // scatte global solution vector x to local vector lx
  // this is not necessary since it had already done in function evaluation
  //VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  //VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);

  // scatte global function vector f to local vector lf
  VecScatterBegin(scatter, f, lf, INSERT_VALUES, SCATTER_FORWARD);
  VecScatterEnd  (scatter, f, lf, INSERT_VALUES, SCATTER_FORWARD);


  VecGetArray(lx, &xx);  // solution value
  VecGetArray(lf, &ff);  // function value

  // do clear
  potential_norm        = 0;
  electron_norm         = 0;
  hole_norm             = 0;
  temperature_norm      = 0;
  elec_temperature_norm = 0;
  hole_temperature_norm = 0;

  poisson_norm              = 0;
  elec_continuity_norm       = 0;
  hole_continuity_norm       = 0;
  heat_equation_norm        = 0;
  elec_energy_equation_norm = 0;
  hole_energy_equation_norm = 0;
  electrode_norm            = 0;

  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    // only consider semiconductor region
    const SimulationRegion * region = _system.region(n);

    switch ( region->type() )
    {
        case SemiconductorRegion :
        {
          unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
          unsigned int node_n_offset   = region->ebm_variable_offset(ELECTRON);
          unsigned int node_p_offset   = region->ebm_variable_offset(HOLE);
          unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);
          unsigned int node_Tn_offset  = region->ebm_variable_offset(E_TEMP);
          unsigned int node_Tp_offset  = region->ebm_variable_offset(H_TEMP);

          SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
          SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
          for(; it!=it_end; ++it)
          {
            const FVM_Node * fvm_node = *it;
            unsigned int offset = fvm_node->local_offset();

            potential_norm   += xx[offset+node_psi_offset]*xx[offset+node_psi_offset];
            electron_norm    += xx[offset+node_n_offset]*xx[offset+node_n_offset];
            hole_norm        += xx[offset+node_p_offset]*xx[offset+node_p_offset];

            poisson_norm        += ff[offset+node_psi_offset]*ff[offset+node_psi_offset];
            elec_continuity_norm += ff[offset+node_n_offset]*ff[offset+node_n_offset];
            hole_continuity_norm += ff[offset+node_p_offset]*ff[offset+node_p_offset];

            if(region->get_advanced_model()->enable_Tl())
            {
              temperature_norm    += xx[offset+node_Tl_offset]*xx[offset+node_Tl_offset];
              heat_equation_norm  += ff[offset+node_Tl_offset]*ff[offset+node_Tl_offset];
            }

            if(region->get_advanced_model()->enable_Tn())
            {
              elec_temperature_norm      += xx[offset+node_Tn_offset]/xx[offset+node_n_offset]*xx[offset+node_Tn_offset]/xx[offset+node_n_offset];
              elec_energy_equation_norm  += ff[offset+node_Tn_offset]*ff[offset+node_Tn_offset];
            }

            if(region->get_advanced_model()->enable_Tp())
            {
              hole_temperature_norm      += xx[offset+node_Tp_offset]/xx[offset+node_p_offset]*xx[offset+node_Tp_offset]/xx[offset+node_p_offset];
              hole_energy_equation_norm  += ff[offset+node_Tp_offset]*ff[offset+node_Tp_offset];
            }
          }
          break;
        }
        case InsulatorRegion :
        case ElectrodeRegion :
        case MetalRegion     :
        {
          unsigned int node_psi_offset = region->ebm_variable_offset(POTENTIAL);
          unsigned int node_Tl_offset  = region->ebm_variable_offset(TEMPERATURE);

          SimulationRegion::const_processor_node_iterator it = region->on_processor_nodes_begin();
          SimulationRegion::const_processor_node_iterator it_end = region->on_processor_nodes_end();
          for(; it!=it_end; ++it)
          {
            const FVM_Node * fvm_node = *it;
            unsigned int offset = fvm_node->local_offset();

            potential_norm   += xx[offset+node_psi_offset]*xx[offset+node_psi_offset];
            poisson_norm     += ff[offset+node_psi_offset]*ff[offset+node_psi_offset];

            if(region->get_advanced_model()->enable_Tl())
            {
              temperature_norm    += xx[offset+node_Tl_offset]*xx[offset+node_Tl_offset];
              heat_equation_norm  += ff[offset+node_Tl_offset]*ff[offset+node_Tl_offset];
            }
          }
          break;
        }
        case VacuumRegion:
        break;
        default: genius_error();
    }
  }


  if(Genius::processor_id() == Genius::n_processors() -1)
    for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
    {
      const BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
      unsigned int offset = bc->local_offset();
      if( offset != invalid_uint )
      {
        potential_norm += xx[offset]*xx[offset];
        electrode_norm += ff[offset]*ff[offset];
      }
    }

  // sum of variable value on all processors
  std::vector<PetscScalar> norm_buffer;

  norm_buffer.push_back(potential_norm);
  norm_buffer.push_back(electron_norm);
  norm_buffer.push_back(hole_norm);
  norm_buffer.push_back(temperature_norm);
  norm_buffer.push_back(elec_temperature_norm);
  norm_buffer.push_back(hole_temperature_norm);

  norm_buffer.push_back(poisson_norm);
  norm_buffer.push_back(elec_continuity_norm);
  norm_buffer.push_back(hole_continuity_norm);
  norm_buffer.push_back(heat_equation_norm);
  norm_buffer.push_back(elec_energy_equation_norm);
  norm_buffer.push_back(hole_energy_equation_norm);
  norm_buffer.push_back(electrode_norm);

  Parallel::sum(norm_buffer);


  // sqrt to get L2 norm
  potential_norm        = sqrt(norm_buffer[0]);
  electron_norm         = sqrt(norm_buffer[1]);
  hole_norm             = sqrt(norm_buffer[2]);
  temperature_norm      = sqrt(norm_buffer[3]);
  elec_temperature_norm = sqrt(norm_buffer[4]);
  hole_temperature_norm = sqrt(norm_buffer[5]);

  poisson_norm              = sqrt(norm_buffer[6]);
  elec_continuity_norm      = sqrt(norm_buffer[7]);
  hole_continuity_norm      = sqrt(norm_buffer[8]);
  heat_equation_norm        = sqrt(norm_buffer[9]);
  elec_energy_equation_norm = sqrt(norm_buffer[10]);
  hole_energy_equation_norm = sqrt(norm_buffer[11]);
  electrode_norm            = sqrt(norm_buffer[12]);

  VecRestoreArray(lx, &xx);
  VecRestoreArray(lf, &ff);

}






///////////////////////////////////////////////////////////////
// provide function and jacobian evaluation for DDML1 solver //
///////////////////////////////////////////////////////////////







/*------------------------------------------------------------------
 * evaluate the residual of function f at x
 */
void EBM3Solver::build_petsc_sens_residual(Vec x, Vec r)
{

  START_LOG("EBM3Solver_Residual()", "EBM3Solver");

  // scatte global solution vector x to local vector lx
  VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);

  PetscScalar *lxx;
  // get PetscScalar array contains solution from local solution vector lx
  VecGetArray(lx, &lxx);

  // clear old data
  VecZeroEntries (r);

  // flag for indicate ADD_VALUES operator.
  InsertMode add_value_flag = NOT_SET_VALUES;

  // evaluate governing equations of DDML1 in all the regions
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Function(lxx, r, add_value_flag);
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // evaluate time derivative if necessary
  if(SolverSpecify::TimeDependent == true)
    for(unsigned int n=0; n<_system.n_regions(); n++)
    {
      SimulationRegion * region = _system.region(n);
      region->EBM3_Time_Dependent_Function(lxx, r, add_value_flag);
    }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // process hanging node here
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Function_Hanging_Node(lxx, r, add_value_flag);
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // preprocess each bc
  VecAssemblyBegin(r);
  VecAssemblyEnd(r);
  std::vector<PetscInt> src_row,  dst_row,  clear_row;
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Function_Preprocess(r, src_row, dst_row, clear_row);
  }
  //add source rows to destination rows, and clear rows
  PetscUtils::VecAddClearRow(r, src_row, dst_row, clear_row);
  add_value_flag = NOT_SET_VALUES;

  // evaluate governing equations of DDML1 for all the boundaries
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Function(lxx, r, add_value_flag);
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // restore array back to Vec
  VecRestoreArray(lx, &lxx);

  // assembly the function Vec
  VecAssemblyBegin(r);
  VecAssemblyEnd(r);

  // scale the function vec
  PetscScalar *ff,*scale;
  // get function array and scale array.
  VecGetArray(r, &ff);
  // L is the scaling vector, the Jacobian evaluate function may dynamically update it.
  VecGetArray(L, &scale);

  // scale it!
  for(unsigned int n=0; n<n_local_dofs; n++)
    ff[n] *= scale[n];

  // restore back
  VecRestoreArray(r, &ff);
  VecRestoreArray(L, &scale);

  STOP_LOG("EBM3Solver_Residual()", "EBM3Solver");

}



/*------------------------------------------------------------------
 * evaluate the Jacobian J of function f at x
 */
void EBM3Solver::build_petsc_sens_jacobian(Vec x, Mat *, Mat *)
{

  START_LOG("EBM3Solver_Jacobian()", "EBM3Solver");

  // scatte global solution vector x to local vector lx
  VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);

  PetscScalar *lxx;
  // get PetscScalar array contains solution from local solution vector lx
  VecGetArray(lx, &lxx);

  MatZeroEntries(J);

  // flag for indicate ADD_VALUES operator.
  InsertMode add_value_flag = NOT_SET_VALUES;

  // evaluate Jacobian matrix of governing equations of EBM in all the regions
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Jacobian(lxx, &J, add_value_flag);
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // evaluate Jacobian matrix of time derivative if necessary
  if(SolverSpecify::TimeDependent == true)
    for(unsigned int n=0; n<_system.n_regions(); n++)
    {
      SimulationRegion * region = _system.region(n);
      region->EBM3_Time_Dependent_Jacobian(lxx, &J, add_value_flag);
    }

  // process hanging node here
  for(unsigned int n=0; n<_system.n_regions(); n++)
  {
    SimulationRegion * region = _system.region(n);
    region->EBM3_Jacobian_Hanging_Node(lxx, &J, add_value_flag);
  }
#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // before first assemble, resereve none zero pattern for each boundary
  if( !jacobian_matrix_first_assemble )
  {
    for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
    {
      BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
      bc->EBM3_Jacobian_Reserve(&J, add_value_flag);
    }
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif

  // evaluate Jacobian matrix of governing equations of DDML2 for all the boundaries
  MatAssemblyBegin(J, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd(J, MAT_FINAL_ASSEMBLY);

  // we do not allow zero insert/add to matrix
  if( !jacobian_matrix_first_assemble )
    genius_assert(!MatSetOption(J, MAT_IGNORE_ZERO_ENTRIES, PETSC_TRUE));


  std::vector<PetscInt> src_row,  dst_row,  clear_row;
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Jacobian_Preprocess(&J, src_row, dst_row, clear_row);
  }

  //add source rows to destination rows
  PetscUtils::MatAddRowToRow(J, src_row, dst_row);
  // clear row
  PetscUtils::MatZeroRows(J, clear_row.size(), clear_row.empty() ? NULL : &clear_row[0], 0.0);
  add_value_flag = NOT_SET_VALUES;
  // evaluate Jacobian matrix of governing equations of EBM for all the boundaries
  for(unsigned int b=0; b<_system.get_bcs()->n_bcs(); b++)
  {
    BoundaryCondition * bc = _system.get_bcs()->get_bc(b);
    bc->EBM3_Jacobian(lxx, &J, add_value_flag);
  }

#if defined(HAVE_FENV_H) && defined(DEBUG)
  genius_assert( !fetestexcept(FE_INVALID) );
#endif


  // restore array back to Vec
  VecRestoreArray(lx, &lxx);

  // assembly the matrix
  MatAssemblyBegin(J, MAT_FINAL_ASSEMBLY);
  MatAssemblyEnd  (J, MAT_FINAL_ASSEMBLY);

  //scaling the matrix
  MatDiagonalScale(J, L, PETSC_NULL);

  // we use the reciprocal of matrix diagonal as scaling value
  // this will take place at next call
  //MatGetDiagonal(J, L);
  //VecReciprocal(L);

  if(!jacobian_matrix_first_assemble)
    jacobian_matrix_first_assemble = true;

  STOP_LOG("EBM3Solver_Jacobian()", "EBM3Solver");

}


void EBM3Solver::set_trace_electrode(BoundaryCondition *bc)
{
  // we needn't scatter again
  // scatte global solution vector x to local vector lx
  //VecScatterBegin(scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  //VecScatterEnd  (scatter, x, lx, INSERT_VALUES, SCATTER_FORWARD);
  bc->EBM3_Electrode_Trace(lx, &J, pdI_pdx, pdF_pdV);
}


