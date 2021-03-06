// $Id: mesh_tools.cc,v 1.3 2008/05/17 05:57:09 gdiso Exp $

// The libMesh Finite Element Library.
// Copyright (C) 2002-2007  Benjamin S. Kirk, John W. Peterson

// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.

// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.

// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA



// C++ includes
#include <set>

// Local includes
#include "mesh_tools.h"
#include "mesh_base.h"
#include "elem.h"
#include "sphere.h"





// ------------------------------------------------------------
// MeshTools functions
unsigned int MeshTools::total_weight(const MeshBase& mesh)
{
  unsigned int weight=0;

  MeshBase::const_element_iterator       el  = mesh.elements_begin();
  const MeshBase::const_element_iterator end = mesh.elements_end();

  for ( ; el != end; ++el)
    weight += (*el)->n_nodes();

  return weight;
}



void MeshTools::build_nodes_to_elem_map (const MeshBase& mesh,
    std::vector<std::vector<unsigned int> >& nodes_to_elem_map)
{
  nodes_to_elem_map.resize (mesh.n_nodes());

  MeshBase::const_element_iterator       el  = mesh.elements_begin();
  const MeshBase::const_element_iterator end = mesh.elements_end();

  for (; el != end; ++el)
    for (unsigned int n=0; n<(*el)->n_nodes(); n++)
    {
      assert ((*el)->node(n) < nodes_to_elem_map.size());
      assert ((*el)->id()    < mesh.n_elem());

      nodes_to_elem_map[(*el)->node(n)].push_back((*el)->id());
    }
}



void MeshTools::build_nodes_to_elem_map (const MeshBase& mesh,
    std::vector<std::vector<const Elem*> >& nodes_to_elem_map)
{
  nodes_to_elem_map.resize (mesh.n_nodes());

  MeshBase::const_element_iterator       el  = mesh.elements_begin();
  const MeshBase::const_element_iterator end = mesh.elements_end();

  for (; el != end; ++el)
    for (unsigned int n=0; n<(*el)->n_nodes(); n++)
    {
      assert ((*el)->node(n) < nodes_to_elem_map.size());

      nodes_to_elem_map[(*el)->node(n)].push_back(*el);
    }
}



void MeshTools::find_boundary_nodes (const MeshBase& mesh,
                                     std::vector<bool>& on_boundary)
{
  // Resize the vector which holds boundary nodes and fill with false.
  on_boundary.resize(mesh.n_nodes());
  std::fill(on_boundary.begin(),
            on_boundary.end(),
            false);

  // Loop over elements, find those on boundary, and
  // mark them as true in on_boundary.
  MeshBase::const_element_iterator       el  = mesh.active_elements_begin();
  const MeshBase::const_element_iterator end = mesh.active_elements_end();

  for (; el != end; ++el)
    for (unsigned int s=0; s<(*el)->n_neighbors(); s++)
      if ((*el)->neighbor(s) == NULL) // on the boundary
      {
        const AutoPtr<Elem> side((*el)->build_side(s));

        for (unsigned int n=0; n<side->n_nodes(); n++)
          on_boundary[side->node(n)] = true;
      }
}



MeshTools::BoundingBox MeshTools::bounding_box(const MeshBase& mesh)
{
  // processor bounding box with no arguments
  // computes the global bounding box
  return processor_bounding_box(mesh);
}



Sphere MeshTools::bounding_sphere(const MeshBase& mesh)
{
  BoundingBox bbox = bounding_box(mesh);

  const Real  diag = (bbox.second - bbox.first).size();
  const Point cent = (bbox.second + bbox.first)/2.;

  return Sphere (cent, .5*diag);
}



MeshTools::BoundingBox MeshTools::processor_bounding_box (const MeshBase& mesh, const unsigned int pid)
{
  assert (mesh.n_nodes() != 0);

  Point min(1.e30,   1.e30,  1.e30);
  Point max(-1.e30, -1.e30, -1.e30);

  // By default no processor is specified and we compute
  // the bounding box for the whole domain.
  if (pid == invalid_uint)
  {
    for (unsigned int n=0; n<mesh.n_nodes(); n++)
      for (unsigned int i=0; i<mesh.spatial_dimension(); i++)
      {
        min(i) = std::min(min(i), mesh.point(n)(i));
        max(i) = std::max(max(i), mesh.point(n)(i));
      }
  }
  // if a specific processor id is specified then we need
  // to only consider those elements living on that processor
  else
  {
    MeshBase::const_element_iterator       el  = mesh.pid_elements_begin(pid);
    const MeshBase::const_element_iterator end = mesh.pid_elements_end(pid);

    for (; el != end; ++el)
      for (unsigned int n=0; n<(*el)->n_nodes(); n++)
        for (unsigned int i=0; i<mesh.spatial_dimension(); i++)
        {
          min(i) = std::min(min(i), mesh.point((*el)->node(n))(i));
          max(i) = std::max(max(i), mesh.point((*el)->node(n))(i));
        }
  }

  const BoundingBox ret_val(min, max);

  return ret_val;
}



Sphere MeshTools::processor_bounding_sphere (const MeshBase& mesh, const unsigned int pid)
{
  BoundingBox bbox = processor_bounding_box(mesh,pid);

  const Real  diag = (bbox.second - bbox.first).size();
  const Point cent = (bbox.second + bbox.first)/2.;

  return Sphere (cent, .5*diag);
}



MeshTools::BoundingBox MeshTools::subdomain_bounding_box (const MeshBase& mesh, const unsigned int sid)
{
  assert (mesh.n_nodes() != 0);

  Point min( 1.e30,  1.e30,  1.e30);
  Point max(-1.e30, -1.e30, -1.e30);

  // By default no subdomain is specified and we compute
  // the bounding box for the whole domain.
  if (sid == invalid_uint)
  {
    for (unsigned int n=0; n<mesh.n_nodes(); n++)
      for (unsigned int i=0; i<mesh.spatial_dimension(); i++)
      {
        min(i) = std::min(min(i), mesh.point(n)(i));
        max(i) = std::max(max(i), mesh.point(n)(i));
      }
  }

  // if a specific subdomain id is specified then we need
  // to only consider those elements living on that subdomain
  else
  {
    for (unsigned int e=0; e<mesh.n_elem(); e++)
      if (mesh.elem(e)->subdomain_id() == sid)
        for (unsigned int n=0; n<mesh.elem(e)->n_nodes(); n++)
          for (unsigned int i=0; i<mesh.spatial_dimension(); i++)
          {
            min(i) = std::min(min(i), mesh.point(mesh.elem(e)->node(n))(i));
            max(i) = std::max(max(i), mesh.point(mesh.elem(e)->node(n))(i));
          }
  }

  const BoundingBox ret_val(min, max);

  return ret_val;
}



Sphere MeshTools::subdomain_bounding_sphere (const MeshBase& mesh, const unsigned int sid)
{
  BoundingBox bbox = subdomain_bounding_box(mesh,sid);

  const Real  diag = (bbox.second - bbox.first).size();
  const Point cent = (bbox.second + bbox.first)/2.;

  return Sphere (cent, .5*diag);
}



bool MeshTools::in_bounding_box(const std::pair<Point, Point> &b, const Point &p)
{
  return ( p.x() >= b.first.x() && p.x() <= b.second.x() &&
           p.y() >= b.first.y() && p.y() <= b.second.y() &&
           p.z() >= b.first.z() && p.z() <= b.second.z() );
}


bool MeshTools::in_bounding_box(const std::pair<Point, Point> &b, const Point &p, unsigned int d)
{
  switch(d)
  {
    case 0 : return ( p.y() >= b.first.y() && p.y() <= b.second.y() && p.z() >= b.first.z() && p.z() <= b.second.z() );
    case 1 : return ( p.x() >= b.first.x() && p.x() <= b.second.x() && p.z() >= b.first.z() && p.z() <= b.second.z() );
    case 2 : return ( p.x() >= b.first.x() && p.x() <= b.second.x() && p.y() >= b.first.y() && p.y() <= b.second.y() );
  }

  return false;
}



void MeshTools::fill_bounding_box(const std::pair<Point, Point> &b, std::vector<Point> &points)
{
  Point _p1 = b.first;
  Point _p2 = b.second;

  points.resize(8);
  points[0] = _p1;
  points[1] = Point(_p2.x(), _p1.y(), _p1.z());
  points[2] = Point(_p2.x(), _p1.y(), _p2.z());
  points[3] = Point(_p1.x(), _p1.y(), _p2.z());
  points[4] = Point(_p1.x(), _p2.y(), _p1.z());
  points[5] = Point(_p2.x(), _p2.y(), _p1.z());
  points[6] = _p2;
  points[7] = Point(_p1.x(), _p2.y(), _p2.z());
}


Real MeshTools::minimal_distance(const std::pair<Point, Point> &b, const Point & p)
{
  if( in_bounding_box(b, p ) )
  {
    Real d;
    d = std::min(p.x() - b.first.x(), b.second.x()-p.x());
    d = std::min(p.y() - b.first.y(), b.second.y()-p.y());
    d = std::min(p.z() - b.first.z(), b.second.z()-p.z());
    return -d;
  }
  else
  {
    Point MPoint;
    Point _p1 = b.first;
    Point _p2 = b.second;
    MPoint.x() = (p.x() < _p1.x()) ? _p1.x() : (p.x() > _p2.x()) ? _p2.x() : p.x();
    MPoint.y() = (p.y() < _p1.y()) ? _p1.y() : (p.y() > _p2.y()) ? _p2.y() : p.y();
    MPoint.z() = (p.z() < _p1.z()) ? _p1.z() : (p.z() > _p2.z()) ? _p2.z() : p.z();
    return (MPoint - p).size();
  }
  return 0.0;
}


Real MeshTools::minimal_distance(const std::pair<Point, Point> &b1, const std::pair<Point, Point> & b2)
{
  std::vector<Point> vertex1, vertex2;
  fill_bounding_box(b1, vertex1);
  fill_bounding_box(b2, vertex2);

  // inside / intersection
  for(unsigned int n=0; n<8; ++n)
  {
    if( in_bounding_box(b1, vertex2[n]) || in_bounding_box(b2, vertex1[n])) return 0.0;
  }

  Real distance = 1e30;
  // has project intersection
  for(unsigned int n=0; n<8; ++n)
    for(unsigned int d=0; d<3; ++d)
    {
      if( in_bounding_box(b1, vertex2[n], d) || in_bounding_box(b2, vertex1[n], d) )
        distance = std::min(fabs(b1.first(d) - b2.second(d)) , fabs(b1.second(d) - b2.first(d)));
    }

  // no project intersection
  for(unsigned int n=0; n<8; ++n)
  {
    distance = std::min(distance, minimal_distance(b1, vertex2[n]));
    distance = std::min(distance, minimal_distance(b2, vertex1[n]));
  }

  return distance;
}


void MeshTools::elem_types (const MeshBase& mesh,
                            std::vector<ElemType>& et)
{
  MeshBase::const_element_iterator       el  = mesh.elements_begin();
  const MeshBase::const_element_iterator end = mesh.elements_end();

  // Automatically get the first type
  et.push_back((*el)->type());  ++el;

  // Loop over the rest of the elements.
  // If the current element type isn't in the
  // vector, insert it.
  for (; el != end; ++el)
    if (!std::count(et.begin(), et.end(), (*el)->type()))
      et.push_back((*el)->type());
}



unsigned int MeshTools::n_elem_of_type (const MeshBase& mesh,
                                        const ElemType type)
{
  return static_cast<unsigned int>(std::distance(mesh.type_elements_begin(type),
                                   mesh.type_elements_end  (type)));
}



unsigned int MeshTools::n_active_elem_of_type (const MeshBase& mesh,
    const ElemType type)
{
  return static_cast<unsigned int>(std::distance(mesh.active_type_elements_begin(type),
                                   mesh.active_type_elements_end  (type)));
}

unsigned int MeshTools::n_non_subactive_elem_of_type_at_level(const MeshBase& mesh,
    const ElemType type,
    const unsigned int level)
{
  unsigned int cnt = 0;
  // iterate over the elements of the specified type
  MeshBase::const_element_iterator el = mesh.type_elements_begin(type);
  const MeshBase::const_element_iterator end = mesh.type_elements_end(type);

  for(; el!=end; ++el)
    if( ((*el)->level() == level) && !(*el)->subactive())
      cnt++;

  return cnt;
}


unsigned int MeshTools::n_levels(const MeshBase& mesh)
{
  unsigned int max_level = 0;

  MeshBase::const_element_iterator el = mesh.active_elements_begin();
  const MeshBase::const_element_iterator end_el = mesh.active_elements_end();

  for( ; el != end_el; ++el)
    max_level = std::max((*el)->level(), max_level);

  return max_level;
}



void MeshTools::get_not_subactive_node_ids(const MeshBase& mesh,
    std::set<unsigned int>& not_subactive_node_ids)
{
  MeshBase::const_element_iterator el           = mesh.elements_begin();
  const MeshBase::const_element_iterator end_el = mesh.elements_end();
  for( ; el != end_el; ++el)
  {
    Elem* elem = (*el);
    if(!elem->subactive())
      for (unsigned int n=0; n<elem->n_nodes(); ++n)
        not_subactive_node_ids.insert(elem->node(n));
  }
}



unsigned int MeshTools::n_elem (MeshBase::element_iterator& begin,
                                MeshBase::element_iterator& end)
{
  return std::distance(begin, end);
}

void MeshTools::find_nodal_neighbors(const MeshBase&, const Node& n,
                                     std::vector<std::vector<const Elem*> >& nodes_to_elem_map,
                                     std::vector<const Node*>& neighbors,
                                     bool active_elements_only)
{
  unsigned int global_id = n.id();

  //Iterators to iterate through the elements that include this node
  std::vector<const Elem*>::const_iterator el     = nodes_to_elem_map[global_id].begin();
  std::vector<const Elem*>::const_iterator end_el = nodes_to_elem_map[global_id].end();

  unsigned int n_ed=0; //Number of edges on the element
  unsigned int ed=0; //Current edge
  unsigned int l_n=0; //Local node number
  unsigned int o_n=0; //Other node on this edge

  //Assume we find a edge... then prove ourselves wrong...
  bool found_edge=true;

  const Node * node_to_save = NULL;
  neighbors.clear();

  //Look through the elements that contain this node
  //find the local node id... then find the side that
  //node lives on in the element
  //next, look for the _other_ node on that side
  //That other node is a "nodal_neighbor"... save it
  for(;el != end_el;el++)
  {
    //We only care about active elements when active_elements_only is true
    if( active_elements_only && !(*el)->active() ) continue;

    {
      n_ed=(*el)->n_edges();

      //Find the local node id
      while(global_id != (*el)->node(l_n++)) { }
      l_n--; //Hmmm... take the last one back off

      while(ed<n_ed)
      {

        //Find the edge the node is on
        while(found_edge && !(*el)->is_node_on_edge(l_n,ed++))
        {
          //This only happens if all the edges have already been found
          if(ed>=n_ed)
            found_edge=false;
        }

        //Did we find one?
        if(found_edge)
        {
          ed--; //Take the last one back off again

          //Now find the other node on that edge
          while(!(*el)->is_node_on_edge(o_n++,ed) || global_id==(*el)->node(o_n-1)) { }
          o_n--;

          //We've found one!  Save it..
          node_to_save=(*el)->get_node(o_n);

          //Search to see if we've already found this one
          std::vector<const Node*>::const_iterator result = std::find(neighbors.begin(),neighbors.end(),node_to_save);

          //If we didn't find it and add it to the vector
          if(result == neighbors.end())
            neighbors.push_back(node_to_save);
        }

        //Reset to look for another
        o_n=0;

        //Keep looking for edges, node may be on more than one edge
        ed++;
      }

      //Reset to get ready for the next element
      l_n=ed=0;
      found_edge=true;
    }
  }
}




void MeshTools::find_hanging_nodes_and_parents(const MeshBase& mesh, std::map<unsigned int, std::vector<unsigned int> >& hanging_nodes)
{
  MeshBase::const_element_iterator it  = mesh.active_this_pid_elements_begin();
  const MeshBase::const_element_iterator end = mesh.active_this_pid_elements_end();

  //Loop through all the elements
  for (; it != end; ++it)
  {
    //Save it off for easier access
    const Elem* elem = (*it);

    //Right now this only works for quad4's
    //genius_assert(elem->type() == libMeshEnums::QUAD4);
    if(elem->type() == libMeshEnums::QUAD4)
    {
      //Loop over the sides looking for sides that have hanging nodes
      //This code is inspired by compute_proj_constraints()
      for (unsigned int s=0; s<elem->n_sides(); s++)
      {
        //If not a boundary node
        if (elem->neighbor(s) != NULL)
        {
          // Get pointers to the element's neighbor.
          const Elem* neigh = elem->neighbor(s);

          //Is there a coarser element next to this one?
          if (neigh->level() < elem->level())
          {
            const Elem *ancestor = elem;
            while (neigh->level() < ancestor->level())
              ancestor = ancestor->parent();
            unsigned int s_neigh = neigh->which_neighbor_am_i(ancestor);

            //Couple of helper uints...
            unsigned int node1=0;
            unsigned int node2=0;
            unsigned int hanging_node=0;

            bool found_in_neighbor = false;

            //Find the two vertices that make up this side
          while(!elem->is_node_on_side(node1++,s)) { }
            node1--;

            //Start looking for the second one with the next node
            node2=node1+1;

            //Find the other one
            while(!elem->is_node_on_side(node2++,s)) { }
            node2--;

            //Pull out their global ids:
            node1 = elem->node(node1);
            node2 = elem->node(node2);

            //Now find which node is present in the neighbor
            //FIXME This assumes a level one rule!
            //The _other_ one is the hanging node

            //First look for the first one
            //FIXME could be streamlined a bit
            for(unsigned int n=0;n<neigh->n_sides();n++)
            {
              if(neigh->node(n) == node1)
                found_in_neighbor=true;
            }


            if(!found_in_neighbor)
              hanging_node=node1;
            else //If it wasn't node1 then it must be node2!
              hanging_node=node2;

            //Reset these for reuse
            node1=0;
            node2=0;

            //Find the first node that makes up the side in the neighbor (these should be the parent nodes)
          while(!neigh->is_node_on_side(node1++,s_neigh)) { }
            node1--;

            node2=node1+1;

            //Find the second node...
            while(!neigh->is_node_on_side(node2++,s_neigh)) { }
            node2--;

            //Save them if we haven't already found the parents for this one
            if(hanging_nodes[hanging_node].size()<2)
            {
              hanging_nodes[hanging_node].push_back(neigh->node(node1));
              hanging_nodes[hanging_node].push_back(neigh->node(node2));
            }
          }
        }
      }
    }
  }
}
