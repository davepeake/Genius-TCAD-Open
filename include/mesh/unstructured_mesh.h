// $Id: unstructured_mesh.h,v 1.5 2008/04/24 01:41:51 gdiso Exp $

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



#ifndef __unstructured_mesh_h__
#define __unstructured_mesh_h__

// C++ Includes   -----------------------------------

// Local Includes -----------------------------------
#include "mesh_base.h"



/**
 * The \p UnstructuredMesh class is derived from the \p MeshBase class.  The
 * user will typically want to instantiate and use the
 * Mesh class in her applications, which is currently a simple
 * derived class of UnstructuredMesh.
 * In order to use the adaptive mesh refinment capabilities
 * of the library, first instantiate a MeshRefinement object
 * with a reference to this class.  Then call the appropriate
 * refinement functions from that object.  To interact with the 
 * boundary, instantiate a BoundaryMesh with a reference to
 * this class, and then use that object's functionality.
*/

// ------------------------------------------------------------
// UnstructuredMesh class definition
class UnstructuredMesh : public MeshBase
{
 public:

  /**
   * Constructor.  Requires the dimension and optionally
   * a processor id.  Note that \p proc_id should always
   * be provided for multiprocessor applications.
   */
  UnstructuredMesh (unsigned int d);

  /**
   * Destructor.
   */
  virtual ~UnstructuredMesh();

  /**
   * Converts a mesh with higher-order
   * elements into a mesh with linear elements.  For 
   * example, a mesh consisting of \p Tet10 will be converted
   * to a mesh with \p Tet4 etc.
   */
  virtual void all_first_order ();

  /**
   * Converts a (conforming, non-refined) mesh with linear 
   * elements into a mesh with second-order elements.  For 
   * example, a mesh consisting of \p Tet4 will be converted
   * to a mesh with \p Tet10 etc.  Note that for some elements
   * like \p Hex8 there exist @e two higher order equivalents,
   * \p Hex20 and \p Hex27.  When \p full_ordered is \p true
   * (default), then \p Hex27 is built.  Otherwise, \p Hex20
   * is built.  The same holds obviously for \p Quad4, \p Prism6
   * ...
   */
  virtual void all_second_order (const bool full_ordered=true);  
  
  
  /**
   * Converts all the element in mesh to FVM element. 
   * return true if success. 
   */
  virtual bool all_fvm_elem ();  
  
  /**
   * Generates a new mesh containing all the elements which
   * are assigned to processor \p pid.  This mesh is written
   * to the pid_mesh reference which you must create and pass
   * to the function.
   */
  void create_pid_mesh (UnstructuredMesh& pid_mesh,
			const unsigned int pid) const;
  
  /**
   * Constructs a mesh called "new_mesh" from the current mesh by
   * iterating over the elements between it and it_end and adding
   * them to the new mesh.
   */
  void create_submesh (UnstructuredMesh& new_mesh,
		       const_element_iterator& it,
		       const const_element_iterator& it_end) const;
  

  /**
   * Deep copy of another unstructured mesh class (used by subclass
   * copy constructors)
   */
  virtual void copy_nodes_and_elements(const UnstructuredMesh& other_mesh);  


  /**
   * Other functions from MeshBase requiring re-definition.
   */
  virtual void find_neighbors ();

#ifdef ENABLE_AMR
  /**
   * Delete subactive (i.e. children of coarsened) elements.
   * This removes all elements descended from currently active
   * elements in the mesh.
   */
  virtual bool contract ();
#endif // #ifdef ENABLE_AMR

};


#endif
