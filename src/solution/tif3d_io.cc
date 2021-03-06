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


// C++ includes
#include<map>

// Local includes
#include "tif3d.h"
#include "tif3d_io.h"

#include "mesh_base.h"
#include "boundary_info.h"
#include "mesh_communication.h"
#include "simulation_region.h"
#include "parallel.h"


using PhysicalUnit::cm;
using PhysicalUnit::um;
using PhysicalUnit::V;
using PhysicalUnit::K;
using PhysicalUnit::eV;

/**
 * This method implements reading a mesh from a specified file
 * in TIF3D format.
 */
void TIF3DIO::read (const std::string& filename)
{
  TIF3D tif3d_reader(filename);

  // read tif3d file
  int ierr;
  if( Genius::processor_id() == 0)
    ierr = tif3d_reader.read();

  Parallel::broadcast(ierr);
  if(ierr) genius_error();

  /*
   * after that, we fill mesh structure with mesh information read from TIF
   */
  SimulationSystem & system = FieldInput<SimulationSystem>::system();
  MeshBase & mesh = system.mesh();

  // clear the system
  system.clear();

  // map node * to tif index in TIF::node_array
  std::map<Node *, int>       node_to_tif_index_map;

  if( Genius::processor_id() == 0)
  {

    // fill node location
    std::vector<TIF3D::Node_t>::const_iterator tif_node_it = tif3d_reader.tif_nodes().begin();
    for(int i=0; tif_node_it!=tif3d_reader.tif_nodes().end(); ++i, ++tif_node_it)
    {
      Node * node = mesh.add_point( Point(tif_node_it->x*um, tif_node_it->y*um, tif_node_it->z*um) );
      node_to_tif_index_map[node] = i;
    }

    // fill region label and region material
    // at the same time. set all the remaining boundary edges as "region_neumann"
    std::map<int, int> tif_region_to_mesh_region;
    std::vector<TIF3D::Region_t>::const_iterator tif_region_it  = tif3d_reader.region_array().begin();
    for(int r=0; tif_region_it!=tif3d_reader.region_array().end(); ++tif_region_it)
    {
      std::string material = tif_region_it->material;
      mesh.set_subdomain_label(r, tif_region_it->name );
      mesh.set_subdomain_material(r, material);
      tif_region_to_mesh_region[tif_region_it->index] = r;
      ++r;
    }
    mesh.set_n_subdomains() = tif_region_to_mesh_region.size();


    // build face map
    std::map<TIF3D::Face_t, int, TIF3D::lt_face> face_table;
    {
      std::vector<TIF3D::Face_t>::const_iterator  tif_face_it = tif3d_reader.tif_faces().begin();
      std::vector<TIF3D::Face_t>::const_iterator  tif_face_it_end = tif3d_reader.tif_faces().end();
      for(; tif_face_it!=tif_face_it_end; ++tif_face_it)
      {
        face_table.insert( std::make_pair(*tif_face_it, tif_face_it->bc_index) );
      }
    }



    // fill tets
    const std::vector<TIF3D::Tet_t>  & tif_tets  = tif3d_reader.tif_tets();
    for(unsigned int n=0; n<tif_tets.size(); ++n)
    {
      Elem* elem = mesh.add_elem(Elem::build(TET4).release());

      const TIF3D::Tet_t & tet =  tif_tets[n];
      // tet elem node
      elem->set_node(0) = mesh.node_ptr( tet.c1 );
      elem->set_node(1) = mesh.node_ptr( tet.c2 );
      elem->set_node(2) = mesh.node_ptr( tet.c3 );
      elem->set_node(3) = mesh.node_ptr( tet.c4 );

      // which region this tet belongs to
      elem->subdomain_id() = tif_region_to_mesh_region[tet.region];

      // process tet side
      for(unsigned int n=0; n<4; ++n)
      {
        /*
        int neighbor_region;
        switch(n)
        {
            case 0 : neighbor_region = tif_tets[tet.c4].region; break;
            case 1 : neighbor_region = tif_tets[tet.c3].region; break;
            case 2 : neighbor_region = tif_tets[tet.c1].region; break;
            case 3 : neighbor_region = tif_tets[tet.c2].region; break;
            default : break;
        }

        if( neighbor_region == tet.region ) continue;
        */

        TIF3D::Face_t f;
        f.point1 = elem->get_node(elem->side_node(n, 0))->id();
        f.point2 = elem->get_node(elem->side_node(n, 1))->id();
        f.point3 = elem->get_node(elem->side_node(n, 2))->id();
        if( face_table.find(f) != face_table.end() )
        {
          int bd_index = face_table.find(f)->second;
          mesh.boundary_info->add_side(elem, n, bd_index);
        }
      }
    }

    // map bc_index to bc label
    std::map<const std::string, short int> bd_map;
    typedef std::map<const std::string, short int>::iterator Bd_It;



    //however, the boundary/interface information should be set here



    std::vector<unsigned int>       elems;
    std::vector<unsigned short int> sides;
    std::vector<short int>          bds;

    // get all the boundary element
    mesh.boundary_info->build_side_list (elems, sides, bds);

    //build neighbor information for boundary element. then elem->neighbor() is functional
    mesh.boundary_info->find_neighbors();

    for (size_t nbd=0; nbd<elems.size(); nbd++ )
    {
      // get the element which has boundary/interface side
      const Elem* elem = mesh.elem(elems[nbd]);
      short int bd_index = bds[nbd];

      // face has label
      if(tif3d_reader.face_has_label(bd_index ))
      {
        mesh.boundary_info->remove(elem, sides[nbd]);
        std::string bd_label = tif3d_reader.face_label(bd_index);

        // if the label already exist
        if( bd_map.find(bd_label) != bd_map.end() )
          bd_index = (*bd_map.find(bd_label)).second;
        else
        {
          //else, increase bd_index, insert it into bd_map
          bd_index = bd_map.size() + 1;
          bd_map.insert(std::pair<const std::string, short int>(bd_label,bd_index));
        }

        // add pair-element to boundary with new bd_index
        mesh.boundary_info->add_side(elem, sides[nbd], bd_index);
        continue;
      }

      //is it an interface side
      if( elem->neighbor(sides[nbd])!=NULL )
      {
        // the element and its neighbor should in diffetent subdomain
        unsigned int sbd_id1 = elem->subdomain_id();
        unsigned int sbd_id2 = elem->neighbor(sides[nbd])->subdomain_id();

        // delete the overkilled boundary side
        if (sbd_id1 == sbd_id2)
        {
          mesh.boundary_info->remove(elem, sides[nbd]);
          continue;
        }

        // the side should be an interface side
        genius_assert(elem->on_interface());
        genius_assert(elem->neighbor(sides[nbd])->on_interface());

        //remove the pair-element from boundary
        mesh.boundary_info->remove(elem, sides[nbd]);
        mesh.boundary_info->remove(elem->neighbor(sides[nbd]),
                                   elem->neighbor(sides[nbd])->which_neighbor_am_i(elem));

        // build the label for the interface, which has the form of RegionLabel1_to_RegionLabel2,
        // the two region is alpha ordered.
        std::string bd_label;
        if( tif3d_reader.region(sbd_id1).name < tif3d_reader.region(sbd_id2).name)
          bd_label = tif3d_reader.region(sbd_id1).name + "_to_" + tif3d_reader.region(sbd_id2).name;
        else
          bd_label = tif3d_reader.region(sbd_id2).name + "_to_" + tif3d_reader.region(sbd_id1).name;



        // if the label already exist
        if( bd_map.find(bd_label) != bd_map.end() )
          bd_index = (*bd_map.find(bd_label)).second;
        else
        {
          //else, increase bd_index, insert it into bd_map
          bd_index = bd_map.size() + 1;
          bd_map.insert(std::pair<const std::string, short int>(bd_label,bd_index));
        }

        // add pair-element to boundary with new bd_index
        mesh.boundary_info->add_side(elem, sides[nbd], bd_index);
        mesh.boundary_info->add_side(elem->neighbor(sides[nbd]),
                                     elem->neighbor(sides[nbd])->which_neighbor_am_i(elem),
                                     bd_index);
      }
      // a boundary side
      else
      {
        unsigned int sbd_id = elem->subdomain_id();
        mesh.boundary_info->remove(elem, sides[nbd]);
        std::string bd_label = tif3d_reader.region(sbd_id).name + "_Neumann";

        // if the label already exist
        if( bd_map.find(bd_label) != bd_map.end() )
          bd_index = (*bd_map.find(bd_label)).second;
        else
        {
          //else, increase bd_index, insert it into bd_map
          bd_index = bd_map.size() + 1;
          bd_map.insert(std::pair<const std::string, short int>(bd_label,bd_index));
        }

        // add pair-element to boundary with new bd_index
        mesh.boundary_info->add_side(elem, sides[nbd], bd_index);
      }

    }

    // after the previous remove and insert operation, the number of boundary id maybe changed. renumber it here.
    mesh.boundary_info->rebuild_ids();

    //write down bd labels
    Bd_It bd_it = bd_map.begin();
    for(; bd_it != bd_map.end(); ++bd_it)
    {
      mesh.boundary_info->set_label_to_id( (*bd_it).second, (*bd_it).first );
    }

    // magic number, for 3D mesh, should > 2008
    mesh.magic_num() = 3312;

  }


  /*
   * set mesh structure for all processors, and build simulation system
   */

  // broadcast mesh to all the processor
  MeshCommunication mesh_comm;
  mesh_comm.broadcast(mesh);


  // build simulation system
  system.build_simulation_system();
  system.sync_print_info();

  /*
   * after that, set doping infomation here. this should be done for all the processors
   */

  // node id to TIF index map, this should be same for all processors
  std::map<unsigned int, int> node_id_to_tif_index_map;
  // fill node_id_to_tif_index_map by processor 0
  if( Genius::processor_id() == 0)
  {
    std::map<Node *, int>::iterator it = node_to_tif_index_map.begin();
    for(; it != node_to_tif_index_map.end(); ++it)
      node_id_to_tif_index_map[(*it).first->id()] = (*it).second;
  }

  // broadcast node_id_to_tif_index_map to all the processors
  Parallel::broadcast(node_id_to_tif_index_map , 0);

  // broadcast SolHead_t to all processors
  Parallel::broadcast(tif3d_reader.sol_head().sol_num, 0);
  if(Genius::processor_id() != 0)
    tif3d_reader.sol_head().sol_name_array.resize(tif3d_reader.sol_head().sol_num);
  for(int n=0; n<tif3d_reader.sol_head().sol_num; ++n)
    Parallel::broadcast(tif3d_reader.sol_head().sol_name_array[n]);

  //broadcast SolData to all processors
  unsigned int n_solution = tif3d_reader.sol_data_array().size();
  Parallel::broadcast(n_solution, 0);
  if(Genius::processor_id() != 0)
    tif3d_reader.sol_data_array().resize(n_solution);
  for(unsigned int n=0; n < n_solution; ++n)
  {
    Parallel::broadcast(tif3d_reader.sol_data(n).index);
    Parallel::broadcast(tif3d_reader.sol_data(n).region_index);
    Parallel::broadcast(tif3d_reader.sol_data(n).data_array);
  }

  // ok, we had got enough informations for set up each simulation region
  std::multimap<int,  TIF3D::SolData_t> solution_map;
  typedef std::multimap<int,  TIF3D::SolData_t>::iterator Solution_It;
  for(unsigned int n=0; n<n_solution; ++n)
    solution_map.insert(std::pair<int,  TIF3D::SolData_t>(tif3d_reader.sol_data(n).index, tif3d_reader.sol_data(n)));

  unsigned int donor        = tif3d_reader.sol_head().solution_index("Donor");
  unsigned int acceptor     = tif3d_reader.sol_head().solution_index("Acceptor");

  for(unsigned int r=0; r<system.n_regions(); r++)
  {
    SimulationRegion * region = system.region(r);

    switch ( region->type() )
    {
        case SemiconductorRegion :
        {
          SimulationRegion::local_node_iterator node_it = region->on_local_nodes_begin();
          SimulationRegion::local_node_iterator node_it_end = region->on_local_nodes_end();
          for(; node_it!=node_it_end; ++node_it)
          {
            FVM_Node * fvm_node = (*node_it);
            FVM_NodeData * node_data = fvm_node->node_data();  genius_assert(node_data);

            // tif_node_index is the index in TIF file that this FVM node has
            int tif_node_index = node_id_to_tif_index_map[fvm_node->root_node()->id()];
            // however, one TIF node may has several solution data when it lies on region interface, we should find corrent
            // solution data for this region which has the same region label.
            std::pair<Solution_It, Solution_It> sol_it_pair = solution_map.equal_range(tif_node_index);
            Solution_It sol_it = sol_it_pair.first;
            for(; sol_it!=sol_it_pair.second; ++sol_it)
              if((*sol_it).second.region_index == r) break;

            if( sol_it != sol_it_pair.second && sol_it != solution_map.end())
            {
              // doping
              if(donor!=invalid_uint && acceptor!=invalid_uint)
              {
                node_data->Na()   = (*sol_it).second.data_array[acceptor] * pow(cm, -3);
                node_data->Nd()   = (*sol_it).second.data_array[donor   ] * pow(cm, -3);
              }
            }
          }
          region->init(system.T_external());
          break;
        }
        case InsulatorRegion     :
        {
          region->init(system.T_external());
          break;
        }
        case ElectrodeRegion     :
        {
          region->init(system.T_external());
          break;
        }
        case MetalRegion    :
        {
          region->init(system.T_external());
          break;
        }
        case VacuumRegion        :
        {
          region->init(system.T_external());
          break;
        }
        case PMLRegion           :
        {
          region->init(system.T_external());
          break;
        }
        default: genius_error();
    }
  }

}












