// $Id: edge.h,v 1.2 2008/03/20 13:16:25 gdiso Exp $

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



#ifndef __edge_h__
#define __edge_h__

// C++ includes

// Local includes
#include "elem.h"


// Forward declarations
class Mesh;



/**
 * The \p Edge is an element in 1D. It can be thought of as a
 * line segment.
 */

// ------------------------------------------------------------
// Edge class definition
class Edge : public Elem
{
 public:

  /**
   * Default line element, takes number of nodes and
   * parent. Derived classes implement 'true' elements.
   */
  Edge (const unsigned int nn,
    Elem* p) :
    Elem(nn, Edge::n_sides(), p) {}

  /**
   * Constructor.  Explicitly specifies the number of
   * nodes and neighbors for which storage will be allocated.
   */
  Edge (const unsigned int nn,
    const unsigned int ns,
    Elem* p) :
    Elem(nn, ns, p) {}

  /**
   * @returns 1, the dimensionality of the object.
   */
  unsigned int dim () const { return 1; }

  /**
   * @returns 2. Every edge is guaranteed to have at least 2 nodes.
   */
  unsigned int n_nodes() const { return 2; }

  /**
   * @returns 2
   */
  unsigned int n_sides() const { return 2; }

  /**
   * @returns 2.  Every edge has exactly two vertices.
   */
  unsigned int n_vertices() const { return 2; }

  /**
   * @returns 0.  All 1D elements have no edges.
   */
  unsigned int n_edges() const { return 0; }

  /**
   * @returns 0.  All 1D elements have no faces.
   */
  unsigned int n_faces() const { return 0; }

  /**
   * @returns 2
   */
  unsigned int n_children() const { return 2; }

  /**
   * @returns true iff the specified child is on the
   * specified side
   */
  virtual bool is_child_on_side(const unsigned int c,
                    const unsigned int s) const;

//   /**
//    * @returns 1
//    */
//   unsigned int n_children_per_side(const unsigned int) const { return 1; }

  /**
   * @returns an id associated with the \p s side of this element.
   * The id is not necessariy unique, but should be close.  This is
   * particularly useful in the \p MeshBase::find_neighbors() routine.
   */
  unsigned int key (const unsigned int s) const
  { return this->compute_key(this->node(s)); }

  /**
   * The \p Elem::side() member returns
   * an auto pointer to a NodeElem for the specified node.
   */
  AutoPtr<DofObject> side (const unsigned int i) const;

  /**
   * The \p Elem::side() member returns
   * an auto pointer to a NodeElem for the specified node.
   */
  AutoPtr<Elem> build_side (const unsigned int i, bool proxy=true) const;

  /**
   * The \p Elem::build_edge() member makes no sense for edges.
   */
  AutoPtr<Elem> build_edge (const unsigned int) const
  //{ genius_assert(i < n_nodes()); AutoPtr<DofObject> ap(new Node(*get_node(i))); return ap; }
  { genius_error(); AutoPtr<Elem> ap(NULL); return ap; }


 protected:


#ifdef ENABLE_AMR

  /**
   * Matrix that allows children to inherit boundary conditions.
   */
  unsigned int side_children_matrix (const unsigned int,
                     const unsigned int) const
  { genius_error(); return 0; }

#endif

};





// ------------------------------------------------------------
// Edge class member functions

#endif
