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

//  $Id: boundary_condition.h,v 1.39 2008/07/09 09:10:08 gdiso Exp $

#ifndef __boundary_condition_h__
#define __boundary_condition_h__


#include <vector>
#include <string>

#include "genius_env.h"
#include "log.h"
#include "node.h"
#include "elem.h"
#include "fvm_node_info.h"
#include "external_circuit.h"
#include "petscvec.h"
#include "petscmat.h"
#include "enum_region.h"
#include "enum_bc.h"

//predefine
class FVM_Node;


/**
 *  Boundary type is the geometry type of boundary,
 *  which can be a "boundary" or "interface" of two region with
 *  different subdomain id.
 *  Please don't be confused with boundary condition type,
 *  which only has mathematic/physical property.
 *  The INTER_CONNECT is a special bc type for electrode inter-connect
 */
enum BoundaryType {BOUNDARY, INTERFACE, MIXED_BOUNDARY_INTERFACE, INTER_CONNECT};


/**
 *  @return enum BCType by string
 */
extern BCType BC_string_to_enum(const std::string & str);

/**
 *  @return string BCType by enum
 */
extern std::string BC_enum_to_string(BCType);


/**
 * @return the BCType of an interface boundary by two subdomain material type
 */
extern BCType determine_bc_by_subdomain(const std::string & mat1, const std::string mat2, bool pisces_compatible_mode=false );



// predefine
class SimulationSystem;
class SimulationRegion;

/**
 * The base class of Boundary Condition
 */
class BoundaryCondition
{

public:

  /**
   * constructor
   */
  BoundaryCondition(SimulationSystem  & system, const std::string & label="");

  /**
   * destructor
   */
  virtual ~BoundaryCondition();

  /**
   * @return the const reference to label
   */
  const std::string & label() const
    { return _boundary_name; }

  /**
   * @return writable reference to label
   */
  std::string & label()
  { return _boundary_name; }


  /**
   * @return the boundary id
   */
  short int boundary_id() const
    { return _boundary_id; }


  /**
   * @return the writable reference to boundary id
   */
  short int & boundary_id()
  { return _boundary_id; }


  /**
   * @return the electrode region label. only the "main" bc can own this value.
   * i.e. an electrode Anode has the ohmic contact with region Silicon,
   * then the ohmic bc can be load by Anode instead of its formal name Anode_to_Silicon
   */
  const std::string & electrode_label() const
    { return _electrode_name; }

  /**
   * @return writable reference to electrode region label
   */
  std::string & electrode_label()
  { return _electrode_name; }


  /**
   * add node* with this boundary type into vector _bd_nodes
   */
  void add_node(const Node * node)
  { _bd_nodes.push_back(node); }

  /**
   * set corresponding pointer to (region and FVM_Node) of a boundary Node
   */
  void insert(const Node * node, SimulationRegion *, FVM_Node *);

  /**
   * @return const reference to boundary nodes vector
   */
  const std::vector<const Node *> & nodes() const
    { return _bd_nodes; }

  /**
   * @return boundary nodes number
   */
  unsigned int  n_nodes() const
    { return _bd_nodes.size(); }

  /**
   * @return true when bc contains this node
   */
  bool has_node( const Node * n) const
  { return _bd_fvm_nodes.find(n)!=_bd_fvm_nodes.end(); }


  /**
   * save the (max) two involved regions
   */
  void set_bc_regions(SimulationRegion * r1, SimulationRegion *r2)
  { _bc_regions.first = r1; _bc_regions.second = r2; }

  /**
   * @return the (max) two involved regions
   */
  const std::pair<SimulationRegion *, SimulationRegion *> & bc_regions() const
    { return _bc_regions; }


  typedef std::vector<const Node *>::const_iterator const_node_iterator;


  /**
   * node default const begin() accessor
   */
  const_node_iterator nodes_begin        () const
  {
    return _bd_nodes.begin();
  }

  /**
   * node default const end() accessor
   */
  const_node_iterator nodes_end          () const
  {
    return _bd_nodes.end();
  }


  typedef std::vector<const Node *>::iterator node_iterator;

  /**
   * node default begin() accessor
   */
  node_iterator nodes_begin        ()
  {
    return _bd_nodes.begin();
  }

  /**
   * node default end() accessor
   */
  node_iterator nodes_end          ()
  {
    return _bd_nodes.end();
  }

  /**
   * @return the number of FVM nodes with Node n as its root_node
   */
  unsigned int n_region_node_with_root_node(const Node * n) const
    { return (*_bd_fvm_nodes.find(n)).second.size(); }

  /**
   * @return true if the node on external boundary
   */
  bool node_on_boundary(const Node * n) const
    { return n_region_node_with_root_node(n) == 1; }

  /**
   * @return true if the node on internal interface
   */
  bool node_on_interface(const Node * n) const
    { return n_region_node_with_root_node(n) > 1; }

  /**
   * set bd_id (boundary index) to region FVM_Node,
   * then we can easily find boundary_condition_index for each FVM_Node
   */
  void set_boundary_id_to_fvm_node();


  typedef std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> >::iterator region_node_iterator;

  /**
   * begin() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  region_node_iterator region_node_begin( const Node * n )
  { return  (*_bd_fvm_nodes.find(n)).second.begin(); }

  /**
   * end() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  region_node_iterator region_node_end( const Node * n )
  { return  (*_bd_fvm_nodes.find(n)).second.end(); }


  typedef std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> >::reverse_iterator  region_node_reverse_iterator;

  /**
   * rbegin() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  region_node_reverse_iterator region_node_rbegin( const Node * n )
  { return  (*_bd_fvm_nodes.find(n)).second.rbegin(); }

  /**
   * rend() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  region_node_reverse_iterator region_node_rend( const Node * n )
  { return  (*_bd_fvm_nodes.find(n)).second.rend(); }

  typedef std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> >::const_iterator const_region_node_iterator;

  /**
   * const begin() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  const_region_node_iterator region_node_begin( const Node * n ) const
    { return  (*_bd_fvm_nodes.find(n)).second.begin(); }

  /**
   * const end() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  const_region_node_iterator region_node_end( const Node * n ) const
    { return  (*_bd_fvm_nodes.find(n)).second.end(); }


  typedef std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> >::const_reverse_iterator  const_region_node_reverse_iterator;

  /**
   * rbegin() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  const_region_node_reverse_iterator const_region_node_rbegin( const Node * n ) const
    { return  (*_bd_fvm_nodes.find(n)).second.rbegin(); }

  /**
   * rend() accessor of all the (region and corresponding FVM_Node) of a Node
   */
  const_region_node_reverse_iterator const_region_node_rend( const Node * n ) const
    { return  (*_bd_fvm_nodes.find(n)).second.rend(); }


  /**
   * find the FVM_Node by Node and its region pointer.
   */
  FVM_Node * get_region_fvm_node(const Node * n, const SimulationRegion * region) const;

  /**
   * find the FVM_Node by Node and its region index.
   */
  FVM_Node * get_region_fvm_node(const Node * n, unsigned int subdomain) const;

  /**
   * @return true if boundary node associated with a region with specified SimulationRegionType
   */
  bool has_associated_region(const Node * n, SimulationRegionType rt) const
  {
    typedef std::map<const Node *, std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> > >::const_iterator It;
    It it = _bd_fvm_nodes.find(n);
    return it->second.find(rt)!=it->second.end();
  }

  /**
   * find the FVM_Node by Node and its region type. the region type should be unique in this multimap!
   */
  FVM_Node * get_region_fvm_node(const Node * n, SimulationRegionType type) const
  {
    genius_assert( (*_bd_fvm_nodes.find(n)).second.count(type) == 1 );
    return (*(*_bd_fvm_nodes.find(n)).second.find(type)).second.second;
  }

  /**
   * find the SimulationRegion by Node and its region type. the region type should be unique in this multimap!
   */
  SimulationRegion * get_fvm_node_region(const Node * n, SimulationRegionType type) const
  {
    genius_assert( (*_bd_fvm_nodes.find(n)).second.count(type) == 1 );
    return (*(*_bd_fvm_nodes.find(n)).second.find(type)).second.first;
  }

  /**
   * @return the node neighbor number
   * @Note node n must be on processor node
   */
  unsigned int n_node_neighbors(const Node * n) const;

  /**
   * @return the node neighbor
   * @Note node n must be on processor node
   */
  std::vector<const Node *> node_neighbors(const Node * n) const;

  /**
   * @return boundary type, one of BOUNDARY, INTERFACE, MIXED_BOUNDARY_INTERFACE and INTER_CONNECT
   */
  virtual BoundaryType boundary_type() const=0;


  /**
   * set boundary type
   */
  virtual void set_boundary_type(BoundaryType )  {}


  /**
   * @return boundary condition type
   */
  virtual BCType bc_type() const=0;


  /**
   * @return boundary condition type in string
   */
  virtual std::string bc_type_name() const=0;


  /**
   * @return reference to system
   */
  const SimulationSystem    & system() const
  { return _system; }

  /**
   * @return reference to system
   */
  SimulationSystem    & system()
  { return _system; }


public:

  /**
   * @return the temperature of external entironment.
   */
  virtual PetscScalar T_external() const
    {return _T_Ext;}

  /**
   * @return writable reference to temperature of external entironment
   * however, we should never reach here
   */
  virtual PetscScalar & T_external()
  {return _T_Ext;}

  /**
   * @return a flag to show whether a boundary is full reflection
   * default is false
   */
  virtual bool reflection() const
    { return false; }

  /**
   * @return writable reference to a reflection flag
   * however, we should never reach here
   */
  virtual bool & reflection()
  { genius_error(); return _bool_dummy_;}

  /**
   * @return the heat transfer rate of this boundary
   * however, we should never reach here
   */
  virtual PetscScalar Heat_Transfer() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to heat transfer rate of this boundary
   * however, we should never reach here
   */
  virtual PetscScalar & Heat_Transfer()
  {genius_error(); return _dummy_;}

  /**
   * @return the work function of electrode material
   * however, we should never reach here
   */
  virtual PetscScalar Work_Function() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to work function of electrode material
   * however, we should never reach here
   */
  virtual PetscScalar & Work_Function()
  {genius_error(); return _dummy_;}


  /**
   * @return the thichness of gate material
   * however, we should never reach here
   */
  virtual PetscScalar Thickness() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to thichness of gate material
   * however, we should never reach here
   */
  virtual PetscScalar & Thickness()
  {genius_error(); return _dummy_;}

  /**
   * @return the electric constant of gate material
   * however, we should never reach here
   */
  virtual PetscScalar eps() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to electric constant of gate material
   * however, we should never reach here
   */
  virtual PetscScalar & eps()
  {genius_error(); return _dummy_;}

  /**
   * @return the free charge density.
   * @note it has differente meaning in different BCs
   * however, we should never reach here
   */
  virtual PetscScalar Qf() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to free charge density
   * @note it has differente meaning in different BCs
   * however, we should never reach here
   */
  virtual PetscScalar & Qf()
  {genius_error(); return _dummy_;}


  /**
   * @return the psi of this boundary.
   * however, we should never reach here
   */
  virtual PetscScalar psi() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to psi of this boundary
   * however, we should never reach here
   */
  virtual PetscScalar & psi()
  {genius_error(); return _dummy_;}


  /**
   * @return the current flow of this boundary.
   * however, we should never reach here
   */
  virtual PetscScalar current() const
  {genius_error(); return _dummy_;}

  /**
   * @return writable reference to current flow of this boundary
   * however, we should never reach here
   */
  virtual PetscScalar & current()
  {genius_error(); return _dummy_;}


  /**
   * @return the electron recombination velocity
   * however, we should never reach here
   */
  virtual PetscScalar eRecombVelocity() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to electron recombination velocity
   * however, we should never reach here
   */
  virtual PetscScalar & eRecombVelocity()
  {genius_error(); return _dummy_;}


  /**
  * @return the hole recombination velocity
  * however, we should never reach here
   */
  virtual PetscScalar hRecombVelocity() const
    {genius_error(); return _dummy_;}

  /**
   * @return writable reference to hole recombination velocity
   * however, we should never reach here
   */
  virtual PetscScalar & hRecombVelocity()
  {genius_error(); return _dummy_;}

  /**
   * @return true iff this boundary is an electrode
   */
  virtual bool is_electrode() const=0;


  /**
   * @return true iff this boundary has a current flow
   */
  virtual bool has_current_flow() const=0;

  /**
   * @return the width in z direction
   * for 2D mesh, z_width is the device dimension in Z direction;
   * for 3D mesh, z_width is always 1.0
   */
  virtual PetscScalar z_width() const
    { return _z_width;}

  /**
   * @return the writable reference to width in z direction
   * for 2D mesh, z_width is the device dimension in Z direction;
   * for 3D mesh, z_width is always 1.0
   */
  virtual PetscScalar & z_width()
  { return _z_width;}


public:
  /**
   * @return true if this bc belongs to inter-connect layer
   */
  bool is_inter_connect_bc() const
  {
    return !_inter_connect.empty();
  }

  /**
   * @return const reference of electrodes belongs to this inter_connect
   */
  const std::vector<BoundaryCondition * > & inter_connect() const
    { return _inter_connect; }

  /**
   * @return reference of electrodes belongs to this inter_connect
   */
  std::vector<BoundaryCondition * > & inter_connect()
  { return _inter_connect; }

  /**
   * set inter_connect electrodes
   */
  void set_inter_connect(const std::set<BoundaryCondition * > & bcs)
  {
    std::set<BoundaryCondition * >::const_iterator it;
    for(it=bcs.begin(); it!=bcs.end(); ++it)
      _inter_connect.push_back(*it);
  }

  /**
   * @return true if this bc is the hub of inter-connect layer
   */
  virtual bool is_inter_connect_hub() const
  { return false; }

  /**
   * @return pointer to inter_connect_hub
   */
  BoundaryCondition * inter_connect_hub()
  {return _inter_connect_hub;}

  /**
   * @return pointer to inter_connect_hub
   */
  const BoundaryCondition * inter_connect_hub() const
  {return _inter_connect_hub;}

  /**
   * set inter_connect_hub
   */
  void set_inter_connect_hub(BoundaryCondition * hub)
  { _inter_connect_hub = hub; }

public:

  /**
   * get current solver index
   */
  static unsigned int solver_index()
  { return _solver_index; }


  /**
   * set solver index, we allow max 4 solvers exist at the same time
   */
  static void set_solver_index(unsigned int s)
  {
    genius_assert(s < 4);
    _solver_index = s;
  }

  /**
   * @return the offset of nodal solution data in global petsc vector
   */
  unsigned int global_offset () const
    { return _global_offset[_solver_index]; }

  /**
   * function for set global offset
   */
  void set_global_offset (unsigned int pos )
  { _global_offset[_solver_index] = pos; }


  /**
   * @return the offset of nodal solution data in local vector
   */
  unsigned int local_offset () const
    { return _local_offset[_solver_index]; }

  /**
   * function for set local offset
   */
  void set_local_offset (unsigned int pos )
  { _local_offset[_solver_index] = pos; }

  /**
   * @return the offset of nodal solution data in array derived from VecGetArray
   */
  unsigned int array_offset () const
    { return _array_offset[_solver_index]; }

  /**
   * function for set array offset
   */
  void set_array_offset (unsigned int pos )
  { _array_offset[_solver_index] = pos; }


  /**
   * let this bc hold a pointer of External Circuit
   */
  void build_ext_circuit(ExternalCircuit * ckt)
  { _ext_circuit = ckt;};


  /**
   * @return the const pointer of External Circuit
   */
  const ExternalCircuit * ext_circuit() const
  {
    return  _ext_circuit;
  }

  /**
   * @return the writable pointer of External Circuit
   */
  ExternalCircuit * ext_circuit()
  {
    return  _ext_circuit;
  }

  /**
   * @return the string which indicates the boundary condition
   */
  virtual std::string boundary_condition_in_string() const
    { return ""; }

  /**
   * derived class can do some more things
   */
  virtual void prepare_for_use() {}

private:

  /**
   * the reference to corresponding SimulationSystem
   * since bc contains physical equations, it is important
   * for bc having the ability to access region information
   */
  SimulationSystem    & _system;

  /**
   * the boundary name given by user
   */
  std::string _boundary_name;

  /**
   * the boundary id
   */
  short int  _boundary_id;

  /**
   * the nodes this boundary/interface has
   * @Note: please make sure the nodes are sorted by their id
   */
  std::vector<const Node *> _bd_nodes;

  /**
   * record (max) two regions this bc involved
   */
  std::pair<SimulationRegion *, SimulationRegion *> _bc_regions;

  /**
   * the global node to region node map. the regions are sorted by SimulationRegionType
   */
  std::map<const Node *, std::multimap<SimulationRegionType, std::pair<SimulationRegion *, FVM_Node *> > > _bd_fvm_nodes;


  /**
   * the electrode region name, which can be used to specify the
   * electrode boundary
   */
  std::string _electrode_name;

  /**
   * pointer to External Circuit, only electrode owns this data
   */
  ExternalCircuit * _ext_circuit;

  /**
   * the width in z direction
   * for 2D mesh, z_width is the device dimension in Z direction;
   * for 3D mesh, z_width is always 1.0
   * @Note system level also has z.width variable.
   * However, it will be override by boundary level z.width
   */
  PetscScalar _z_width;


  /**
   * temperature of external entironment
   */
  PetscScalar   _T_Ext;

  /**
   * An inter-connect layer of IC, it can connect several electrodes
   * every electrodes belongs to this inter-connect layer owns the same _inter_connect structure
   */
  std::vector<BoundaryCondition * > _inter_connect;

  /**
   * pointer to _inter_connect_hub
   * every electrodes belongs to this inter-connect layer owns this pointer
   */
  BoundaryCondition * _inter_connect_hub;


  /**
   * this variable determines which _global_offset/_local_offset pair are used
   * default is 0, max is 7. that means we can use up to 8 individual solvers
   * each has their own _global_offset/_local_offset pair
   */
  static unsigned int _solver_index;

  /**
   * the offset of bc equation in global petsc vector
   * this value should be set on each processor
   * different solver may set this variable with different value
   */
  unsigned int _global_offset[4];

  /**
   * the offset of bc equation in local petsc vector
   * (scattered from global petsc vector with VecScatterBegin/VecScatterEnd)
   * this value should be set on each processor
   * different solver may set this variable with different value
   */
  unsigned int _local_offset[4];

  /**
   * some times, we need to access bc variable in a \p global vector
   * with array generated by VecGetArray/VecRestoreArray
   */
  unsigned int _array_offset[4];

private:

  /**
   * dummy to prevent compile problem
   */
  static PetscScalar _dummy_;

  /**
   * the same purpose with dummy
   */
  static bool _bool_dummy_;

public:

  //////////////////////////////////////////////////////////////////////////////////////////////
  //----------------Function and Jacobian evaluate for Poisson's Equation---------------------//
  //////////////////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for fill vector of poisson's equation.
   *
   * filling solution data into petsc vector of poisson's equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void Poissin_Fill_Value(Vec , Vec ) {}


  /**
   * @brief virtual function for preprocess of poisson's equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void Poissin_Function_Preprocess(Vec /*f*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}


  /**
   * @brief virtual function for evaluating poisson's equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void Poissin_Function(PetscScalar * x, Vec f, InsertMode &add_value_flag)=0;


  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition can override it
   */
  virtual void Poissin_Jacobian_Reserve(Mat *, InsertMode &) {}

  /**
   * @brief virtual function for preprocess Jacobian Matrix of poisson's equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void Poissin_Jacobian_Preprocess(Mat */*jac*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating Jacobian of poisson's equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void Poissin_Jacobian(PetscScalar * x, Mat *jac, InsertMode &add_value_flag)=0;

  /**
   * @brief virtual function for update solution value of poisson's equation.
   *
   *
   * @param PetscScalar*     global solution vector
   *
   * @note do nothing, each derived boundary condition can override it
   */
  virtual void Poissin_Update_Solution(PetscScalar *) {}



  //////////////////////////////////////////////////////////////////////////////////
  //----------------Function and Jacobian evaluate for L1 DDM---------------------//
  //////////////////////////////////////////////////////////////////////////////////


  /**
   * @brief virtual function for fill vector of level 1 DDM equation.
   *
   * filling solution data into petsc vector of level 1 DDM equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Fill_Value(Vec , Vec ) {}

  /**
   * @brief virtual function for preprocess for level 1 DDM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Function_Preprocess(Vec /*f*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}


  /**
   * @brief virtual function for evaluating level 1 DDM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM1_Function(PetscScalar * x, Vec f, InsertMode &add_value_flag)=0;


  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat*              petsc global jacobian matrix
   * @param InsertMode&       flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Jacobian_Reserve(Mat *, InsertMode &) {}

  /**
   * @brief virtual function for preprocess Jacobian Matrix of level 1 DDM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Jacobian_Preprocess(Mat */*jac*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating Jacobian of level 1 DDM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM1_Jacobian(PetscScalar * x, Mat *jac, InsertMode &add_value_flag)=0;

  /**
   * @brief virtual function for evaluating trace parameter of level 1 DDM equation.
   *
   * @param Vec              local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param Vec              vector for dI/dx
   * @param Vec              vector for dF/dV
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM1_Electrode_Trace(Vec, Mat *, Vec , Vec) {}

  /**
   * @brief virtual function for update solution value of level 1 DDM equation.
   *
   * @param PetscScalar*     global solution vector
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Update_Solution(PetscScalar *) {}


  /**
   * @brief virtual function for post process of level 1 DDM equation.
   *
   * @param PetscScalar*     global solution vector
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM1_Post_Process() {}


  //////////////////////////////////////////////////////////////////////////////////
  //----------Function and Jacobian evaluate for Advanced Mixed DDML1-------------//
  //////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for fill vector for Advanced Mixed type level 1 DDM equation.
   *
   * filling solution data into petsc vector of Advanced Mixed type level 1 DDM equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM1_Fill_Value(Vec x, Vec L)
  { this->DDM1_Fill_Value(x, L); }

  /**
   * @brief virtual function for preprocess for Advanced Mixed type level 1 DDM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM1_Function_Preprocess(Vec f, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->DDM1_Function_Preprocess(f, src, dst, clear); }

  /**
   * @brief virtual function for evaluating Advanced Mixed type level 1 DDM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM1_Function(PetscScalar *x , Vec f, InsertMode &add_value_flag)
  { this->DDM1_Function(x , f, add_value_flag ); }

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat *              petsc global jacobian matrix
   * @param InsertMode&        flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM1_Jacobian_Reserve(Mat * jac, InsertMode &add_value_flag)
  { this->DDM1_Jacobian_Reserve(jac, add_value_flag ); }


  /**
   * @brief virtual function for preprocess Jacobian Matrix for Advanced Mixed type level 1 DDM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM1_Jacobian_Preprocess(Mat *jac, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->DDM1_Jacobian_Preprocess(jac, src, dst, clear); }

  /**
   * @brief virtual function for evaluating Advanced Mixed type Jacobian of level 1 DDM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM1_Jacobian(PetscScalar * x, Mat * jac, InsertMode &add_value_flag)
  { this->DDM1_Jacobian(x, jac, add_value_flag); }


  //////////////////////////////////////////////////////////////////////////////////
  //----------------Function and Jacobian evaluate for L2 DDM---------------------//
  //////////////////////////////////////////////////////////////////////////////////


  /**
   * @brief virtual function for fill vector of level 2 DDM equation.
   *
   * filling solution data into petsc vector of level 2 DDM equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void DDM2_Fill_Value(Vec , Vec ) {}

  /**
   * @brief virtual function for preprocess for level 2 DDM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM2_Function_Preprocess(Vec /*f*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating level 2 DDM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM2_Function(PetscScalar * x, Vec f, InsertMode &add_value_flag)=0;

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat *              petsc global jacobian matrix
   * @param InsertMode&        flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition can override it
     */
  virtual void DDM2_Jacobian_Reserve(Mat *, InsertMode &) {}

  /**
   * @brief virtual function for preprocess Jacobian Matrix of level 2 DDM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
     */
  virtual void DDM2_Jacobian_Preprocess(Mat */*jac*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating Jacobian of level 2 DDM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM2_Jacobian(PetscScalar * x, Mat *jac, InsertMode &add_value_flag)=0;

  /**
   * @brief virtual function for evaluating trace parameter of level 2 DDM equation.
   *
   * @param Vec              local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param Vec              vector for dI/dx
   * @param Vec              vector for dF/dV
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void DDM2_Electrode_Trace(Vec, Mat *, Vec , Vec) {}

  /**
   * @brief virtual function for update solution value of level 2 DDM equation.
   *
   * @param PetscScalar*     global solution vector
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDM2_Update_Solution(PetscScalar *) {}


  //////////////////////////////////////////////////////////////////////////////////
  //----------Function and Jacobian evaluate for Advanced Mixed DDML2-------------//
  //////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for fill vector for Advanced Mixed type level 2 DDM equation.
   *
   * filling solution data into petsc vector of Advanced Mixed type level 1 DDM equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM2_Fill_Value(Vec x, Vec L)
  { this->DDM2_Fill_Value(x, L); }

  /**
   * @brief virtual function for preprocess for Advanced Mixed type level 2 DDM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM2_Function_Preprocess(Vec f, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->DDM2_Function_Preprocess(f, src, dst, clear); }

  /**
   * @brief virtual function for evaluating Advanced Mixed type level 2 DDM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM2_Function(PetscScalar *x , Vec f, InsertMode &add_value_flag)
  { this->DDM2_Function(x , f, add_value_flag ); }

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat *              petsc global jacobian matrix
   * @param InsertMode&        flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM2_Jacobian_Reserve(Mat * jac, InsertMode &add_value_flag)
  { this->DDM2_Jacobian_Reserve(jac, add_value_flag ); }

  /**
   * @brief virtual function for preprocess Jacobian Matrix for Advanced Mixed type level 2 DDM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_DDM2_Jacobian_Preprocess(Mat *jac, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->DDM2_Jacobian_Preprocess(jac, src, dst, clear); }

  /**
   * @brief virtual function for evaluating Advanced Mixed type Jacobian of level 2 DDM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_DDM2_Jacobian(PetscScalar * x, Mat * jac, InsertMode &add_value_flag)
  { this->DDM2_Jacobian(x, jac, add_value_flag); }


  //////////////////////////////////////////////////////////////////////////////////
  //----------------Function and Jacobian evaluate for Hybrid EBM-----------------//
  //////////////////////////////////////////////////////////////////////////////////


  /**
   * @brief virtual function for fill vector of level 3 EBM equation.
   *
   * filling solution data into petsc vector of level 3 EBM equation.
   * can be used as initial data of nonlinear equation or diverged recovery.
   *
   * @param Vec              global solution vector
   * @param Vec              the left scaling vector
   * @note each derived boundary condition can override it
   */
  virtual void EBM3_Fill_Value(Vec , Vec ) {}

  /**
   * @brief virtual function for preprocess for level 3 EBM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void EBM3_Function_Preprocess(Vec /*f*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating level 3 EBM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void EBM3_Function(PetscScalar *, Vec , InsertMode &) {}

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat *              petsc global jacobian matrix
   * @param InsertMode&        flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition can override it
   */
  virtual void EBM3_Jacobian_Reserve(Mat *, InsertMode &) {}

  /**
   * @brief virtual function for preprocess Jacobian Matrix of level 3 EBM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void EBM3_Jacobian_Preprocess(Mat */*jac*/, std::vector<PetscInt> &/*src*/,  std::vector<PetscInt> &/*dst*/, std::vector<PetscInt> &/*clear*/) {}

  /**
   * @brief virtual function for evaluating Jacobian of level 3 EBM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void EBM3_Jacobian(PetscScalar * , Mat *, InsertMode &) {}

  /**
   * @brief virtual function for evaluating trace parameter of level 3 EBM equation.
   *
   * @param Vec              local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param Vec              vector for dI/dx
   * @param Vec              vector for dF/dV
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition should override it
   */
  virtual void EBM3_Electrode_Trace(Vec, Mat *, Vec , Vec) {}

  /**
   * @brief virtual function for update solution value of level 3 EBM equation.
   *
   * @param PetscScalar*     global solution vector
   *
   * @note each derived boundary condition can override it
   */
  virtual void EBM3_Update_Solution(PetscScalar *) {}



  //////////////////////////////////////////////////////////////////////////////////
  //----------Function and Jacobian evaluate for Advanced Mixed EBM3 -------------//
  //////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for evaluating Advanced Mixed type level 3 EBM equation.
   *
   * @param PetscScalar*    local unknown vector
   * @param Vec             petsc global function vector
   * @param InsertMode&     flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_EBM3_Function(PetscScalar *x , Vec f, InsertMode &add_value_flag)
  { this->EBM3_Function(x , f, add_value_flag ); }

  /**
   * @brief virtual function for preprocess for Advanced Mixed type level 3 EBM equation.
   *
   * @param f            petsc global function vector
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_EBM3_Function_Preprocess(Vec f, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->EBM3_Function_Preprocess(f, src, dst, clear); }

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat *              petsc global jacobian matrix
   * @param InsertMode&        flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_EBM3_Jacobian_Reserve(Mat * jac, InsertMode &add_value_flag)
  { this->EBM3_Jacobian_Reserve(jac, add_value_flag ); }


  /**
   * @brief virtual function for preprocess Jacobian Matrix for Advanced Mixed type level 3 EBM equation.
   *
   * @param jac          petsc global jacobian matrix
   * @param src          source row
   * @param dst          destination row
   * @param clear        row for clear
   *
   * @note each derived boundary condition can override it
   */
  virtual void MixA_EBM3_Jacobian_Preprocess(Mat *jac, std::vector<PetscInt> &src,  std::vector<PetscInt> &dst, std::vector<PetscInt> &clear)
  { this->EBM3_Jacobian_Preprocess(jac, src, dst, clear); }

  /**
   * @brief virtual function for evaluating Advanced Mixed type Jacobian of level 3 EBM equation.
   *
   * @param PetscScalar*     local unknown vector
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void MixA_EBM3_Jacobian(PetscScalar * x, Mat * jac, InsertMode &add_value_flag)
  { this->EBM3_Jacobian(x, jac, add_value_flag); }


  //////////////////////////////////////////////////////////////////////////////////
  //--------------Matrix and RHS Vector evaluate for DDM AC Solver----------------//
  //////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for evaluating matrix and rhs vector for ddm ac solver
   *
   * @param Mat              petsc AC matrix
   * @param Vec              rhs vector
   * @param Mat              petsc global jacobian matrix
   * @param double           AC frequency
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note only electrode boundary need to override it
   */
  virtual void DDMAC_Fill_Matrix_Vector( Mat,  Vec, const Mat, const double, InsertMode &  ) {}

  /**
   * @brief virtual function for update solution value for ddm ac solver
   *
   * @param PetscScalar*     local solution vector
   * @param Mat              petsc global jacobian matrix
   *
   * @note each derived boundary condition can override it
   */
  virtual void DDMAC_Update_Solution(const PetscScalar * , const Mat, const double) {}


  //////////////////////////////////////////////////////////////////////////////////
  //----------------- functions for Fast Hydrodynamic solver  --------------------//
  //////////////////////////////////////////////////////////////////////////////////


  /**
   * @brief virtual function for calculate ghost cell volume for HDM method
   *
   * @param Vec              volume vector
   *
   * @note only semiconductor neumann-boundary/insulator-interface need to override it
   */
  virtual void HDM_Ghostcell_Volume( Vec /*vol*/ ) {}

  /**
   * @brief virtual function for evaluating boundary for HDM method
   *
   * @param PetscScalar*     local solution vector
   * @param Vec              solution vector
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note only semiconductor boundary need to override it
   */
  virtual void HDM_Boundary( const PetscScalar * , Vec /*x*/, InsertMode &  ) {}


  //////////////////////////////////////////////////////////////////////////////////
  //-----------------  functions for Linear Poissin solver   ---------------------//
  //////////////////////////////////////////////////////////////////////////////////

  /**
   * @brief virtual function for reserve none zero pattern in petsc matrix.
   *
   * @param Mat*             petsc global jacobian matrix
   * @param InsertMode&      flag for last operator is ADD_VALUES
   *
   * @note each derived boundary condition can override it
   */
  virtual void LinearPoissin_Reserve(Mat A, InsertMode &) {}

  /**
   * @brief virtual function for build matrix of linear poisson's equation.
   *
   * @param A                matrix
   *
   * @note each derived region should override it
   */
  virtual void LinearPoissin_Matrix(Mat A, InsertMode &) {}


  /**
   * @brief virtual function for build RHS vector of linear poisson's equation.
   *
   * @param b                RHS vector
   *
   * @note each derived region should override it
   */
  virtual void LinearPoissin_RHS(Vec b, InsertMode &) {}
}
;


#endif

