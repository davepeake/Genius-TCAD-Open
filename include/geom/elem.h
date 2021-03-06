// $Id: elem.h,v 1.21 2008/06/11 07:01:24 gdiso Exp $

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



#ifndef __elem_h__
#define __elem_h__

// C++ includes
#include <algorithm>
#include <set>
#include <vector>

// Local includes
#include "genius_common.h"
#include "dof_object.h"
#include "node.h"
#include "elem_intersection.h"
#include "vector_value.h"
#include "enum_elem_type.h"
#include "enum_elem_quality.h"
#include "enum_io_package.h"
#include "enum_order.h"
#include "auto_ptr.h"
#include "multi_predicates.h"
#include "variant_filter_iterator.h"

// Forward declarations
class MeshRefinement;
class Elem;
class FVM_Node;


/**
 * This is the base class from which all geometric entities
 * (elements) are derived.  The \p Elem class contains information
 * that every entity might need, such as its number of nodes and
 * pointers to the nodes to which it is connected.  This class
 * also provides virtual functions that will be overloaded by
 * derived classes.  These functions provide information such as
 * the number of sides the element has, who its neighbors are,
 * how many children it might have, and who they are.
 *
 * In an \p Elem becomes an \p Edge in 1D, a \p Face in 2D, and a \p
 * Cell in 3D.  An \p Elem is composed of a number of sides, which you
 * may access as \p Elem types in dimension \p D-1.  For example, a
 * concrete element type in 3D is a \p Hex8, which is a hexahedral. A
 * \p Hex8 has 6 sides, which are \p Faces.  You may access these
 * sides.
 *
 * An \p Elem is composed of a number of \p Node objects.  Some of
 * these nodes live at the vertices of the element, and others may
 * live on edges (and faces in 3D) or interior to the element.  The
 * number of vertices an element contains \p n_vertices() is
 * determined strictly by the type of geometric object it corresponds
 * to.  For example, a \p Tri is a type of \p Face that always
 * contains 3 vertices.  A \p Tri3 is a specific triangular element
 * type with three 3 nodes, all located at the vertices.  A \p Tri6 is
 * another triangular element with 6 nodes, 3 of which are located at
 * vertices and another 3 that live on the edges.
 * In all that follows, nodes that live either on edges, faces or the
 * interior are named @e second-order nodes.
 *
 * \author Benjamin S. Kirk, 2002-2007
 */

// ------------------------------------------------------------
// Elem class definition

class Elem :    public DofObject
{
 protected:

  /**
   * Constructor.  Creates an element with \p n_nodes nodes,
   * \p n_sides sides, \p n_children possible children, and
   * parent \p p.  The constructor allocates the memory necessary
   * to support this data.
   */
  Elem (const unsigned int n_nodes=0,
        const unsigned int n_sides=0,
        Elem* parent=NULL);

 public:

  /**
   * Destructor.  Frees all the memory associated with the element.
   */
  virtual ~Elem();

  /**
   * @returns the \p Point associated with local \p Node \p i.
   */
  virtual const Point & point (const unsigned int i) const;

  /**
   * @returns the \p Point associated with local \p Node \p i
   * as a writable reference.
   */
  virtual Point & point (const unsigned int i);

  /**
   * @returns the global id number of local \p Node \p i.
   */
  virtual unsigned int node (const unsigned int i) const;

  /**
   * @returns the pointer to local \p Node \p i.
   */
  virtual Node* get_node (const unsigned int i) const;


  /**
   * set the \p ith FVM_Node pointer.
   * only for FVM element
   */
  virtual void hold_fvm_node(const unsigned int , FVM_Node *)
  {genius_error(); return; }


  /**
   * @returns the pointer to local \p FVM_Node \p i.
   * only for FVM element
   */
  virtual FVM_Node * get_fvm_node(const unsigned int /* i */) const
  { genius_error(); return NULL; }


  /**
   * @returns the pointer to local \p FVM_Node \p i on side \p s.
   * only for FVM element
   */
  virtual FVM_Node * get_side_fvm_node(const unsigned int /* s */, const unsigned int /* i */) const
  { genius_error(); return NULL; }



  /**
   * @returns the pointer to local \p Node \p i as a writable reference.
   */
  virtual Node* & set_node (const unsigned int i);

  /**
   * @returns the subdomain that this element belongs to.
   */
  unsigned int subdomain_id () const;

  /**
   * @returns the subdomain that this element belongs to as a
   * writable reference.
   */
  unsigned int & subdomain_id ();

  /**
   * @returns the subdomain of my ith neighbor element belongs to.
   */
  unsigned int subdomain_id (const unsigned int i) const;

  /**
   * @returns an id assocated with this element.  The id is not
   * guaranteed to be unique, but it should be close.  The id
   * is thus useful, for example, as a key in a hash table
   * data structure.
   * for 1d elem (edge), the key equals to 2d elem (tri, quad)'s side key
   * for 2d elem (tri, quad), the key equals to 3d elem (tet, prism, etc)'s side key
   * for 3d elem, equal to the sum's of all the side's key
   */
  unsigned int key () const;

  /**
   * @returns an id associated with the \p s side of this element.
   * The id is not necessariy unique, but should be close.  This is
   * particularly useful in the \p MeshBase::find_neighbors() routine.
   */
  virtual unsigned int key (const unsigned int s) const = 0;

  /**
   * @returns true if two elements are identical, false otherwise.
   * This is true if the elements are connected to identical global
   * nodes, regardless of how those nodes might be numbered local
   * to the elements.
   */
  virtual bool operator == (const DofObject& rhs) const;

  /**
   * @returns a pointer to the \f$ i^{th} \f$ neighbor of this element.
   * If \p MeshBase::find_neighbors() has not been called this
   * simply returns \p NULL.  If \p MeshBase::find_neighbors()
   * has been called and this returns \p NULL then the side is on
   * a boundary of the domain.
   */
  Elem* neighbor (const unsigned int i) const;

  /**
   * Assigns \p n as the \f$ i^{th} \f$ neighbor.
   */
  void set_neighbor (const unsigned int i, Elem* n);

  /**
   * @returns \p true if the element \p elem in question is a neighbor
   * of this element, \p false otherwise.
   */
  bool is_neighbor (const Elem* elem) const;

  /**
   * If the element \p elem in question is a neighbor
   * of a child of this element, this returns a pointer
   * to that child.  Otherwise it returns NULL.
   */
  Elem* child_neighbor (Elem* elem) const;

  /**
   * If the element \p elem in question is a neighbor
   * of a child of this element, this returns a pointer
   * to that child.  Otherwise it returns NULL.
   */
  const Elem* child_neighbor (const Elem* elem) const;

  /**
   * @returns \p true if this element has a side coincident
   * with a boundary (indicated by a \p NULL neighbor), \p false
   * otherwise.
   */
  bool on_boundary () const;

  /**
   * @returns \p true if this element has a side \p s coincident
   * with a boundary (indicated by a \p NULL neighbor), \p false
   * otherwise.
   */
  bool on_boundary (const unsigned int s) const;


  /**
   * @returns \p true if this element has a side coincident
   * with an interface (indicated by a neighbor with different subdomain_id),
   * \p false otherwise.
   */
  bool on_interface () const;

  /**
   * @returns \p true if this element has a side \p s coincident
   * with an interface (indicated by a neighbor with different subdomain_id),
   * \p false otherwise.
   */
  bool on_interface (const unsigned int s) const;

  /**
   * This function tells you which neighbor you \p (e) are.
   * I.e. if s = a->which_neighbor_am_i(e); then
   * a->neighbor(s) will be an ancestor of e;
   */
  unsigned int which_neighbor_am_i(const Elem *e) const;

  /**
   * This function returns true iff a vertex of e is contained
   * in this element
   */
  bool contains_vertex_of(const Elem *e) const;

  /**
   * This function returns true iff all vertex of e is contained
   * in this element
   */
  bool contains_all_vertex_of(const Elem *e) const;

  /**
   * This function finds all elements which
   * touch the current element at any point
   */
  void find_point_neighbors(std::set<const Elem *> &neighbor_set) const;


  /**
   * This function returns true iff node i and j are neighbors (linked by edge)
   */
  virtual bool node_node_connect(const unsigned int i, const unsigned int j)  const
  {
     genius_error();
     if (i==j)  return true;
     return false;
  }

  /**
   * Returns the connectivity for this element in a specific
   * format, which is specified by the IOPackage tag.  This
   * method supercedes the tecplot_connectivity(...) and vtk_connectivity(...)
   * routines.
   */
  virtual void connectivity(const unsigned int sc,
                            const IOPackage iop,
                            std::vector<unsigned int>& conn) const = 0;

  /**
   * Returns the side order for this element in a specific
   * format, which is specified by the IOPackage tag.
   */
  virtual void side_order( const IOPackage iop, std::vector<unsigned int>& order) const = 0;

  /**
   * write out vtk(?) format for 3D view the element
   */
  virtual void geometri_view(std::ostream & /*out*/) const {}

  /**
   * Writes the element connectivity for various IO packages
   * to the passed ostream "out".  Not virtual, since it is
   * implemented in the base class.  This function supercedes the
   * write_tecplot_connectivity(...) and write_ucd_connectivity(...)
   * routines.
   */
  void write_connectivity (std::ostream& out,
                           const IOPackage iop) const;

//   /**
//    * @returns the VTK element type of the sc-th sub-element.
//    */
//   virtual unsigned int vtk_element_type (const unsigned int sc) const = 0;

  /**
   * @returns the type of element that has been derived from this
   * base class.
   */
  virtual ElemType type () const = 0;

  /**
   * @returns the dimensionality of the object.
   */
  virtual unsigned int dim () const = 0;

  /**
   * @returns the number of nodes this element contains.
   */
  virtual unsigned int n_nodes () const = 0;

  /**
   * @returns the number of sides the element that has been derived
   * from this class has. In 2D the number of sides is the number
   * of edges, in 3D the number of sides is the number of faces.
   */
  virtual unsigned int n_sides () const = 0;

  /**
   * @returns the number of neighbors the element that has been derived
   * from this class has.  By default only face (or edge in 2D)
   * neighbors are stored, so this method returns n_sides(),
   * however it may be overloaded in a derived class
   */
  virtual unsigned int n_neighbors () const
  { return this->n_sides(); }

  /**
   * @returns the number of vertices the element that has been derived
   * from this class has.
   */
  virtual unsigned int n_vertices () const = 0;

  /**
   * @returns the number of edges the element that has been derived
   * from this class has.
   */
  virtual unsigned int n_edges () const = 0;

  /**
   * @returns the number of faces the element that has been derived
   * from this class has.
   */
  virtual unsigned int n_faces () const = 0;

  /**
   * @returns the number of children the element that has been derived
   * from this class may have.
   */
  virtual unsigned int n_children () const = 0;

  /**
   * @returns true iff the specified (local) node number is a vertex.
   */
  virtual bool is_vertex(const unsigned int i) const = 0;

  /**
   * @returns true iff the specified (local) node number is an edge.
   */
  virtual bool is_edge(const unsigned int i) const = 0;

  /**
   * @returns true iff the specified (local) node number is a face.
   */
  virtual bool is_face(const unsigned int i) const = 0;

  /**
   * @returns true iff the specified (local) node number is on the
   * specified side
   */
  virtual bool is_node_on_side(const unsigned int n,
                               const unsigned int s) const = 0;

  /**
   * @returns true iff the specified (local) node number is on the
   * specified edge
   */
  virtual bool is_node_on_edge(const unsigned int n,
                               const unsigned int e) const = 0;

  /**
   * get the node local index on edge e
   */
  virtual void nodes_on_edge (const unsigned int e,
                              std::vector<unsigned int> & nodes ) const =0;

  /**
   * get the node local index on edge2 e
   */
  virtual void nodes_on_edge (const unsigned int e,
                              std::pair<unsigned int, unsigned int> & nodes ) const =0;

  /**
   * @returns true iff the specified (local) edge number is on the
   * specified side
   */
  virtual bool is_edge_on_side(const unsigned int e,
                               const unsigned int s) const = 0;

//   /**
//    * @returns the number of children this element has that
//    * share side \p s
//    */
//   virtual unsigned int n_children_per_side (const unsigned int) const = 0;

  /**
   * @returns the number of sub-elements this element may be broken
   * down into for visualization purposes.  For example, this returns
   * 1 for a linear triangle, 4 for a quadratic (6-noded) triangle, etc...
   */
  virtual unsigned int n_sub_elem () const = 0;

  /**
   * @returns a proxy element coincident with side \p i.  This method returns
   * the _minimum_ element necessary to uniquely identify the side.  So,
   * for example, the side of a hexahedral is always returned as a 4-noded
   * quadrilateral, regardless of what type of hex you are dealing with.  If
   * you want the full-ordered face (i.e. a 9-noded quad face for a 27-noded
   * hexahedral) use the build_side method.
   */
  virtual AutoPtr<DofObject> side (const unsigned int i) const = 0;

  /**
   * Creates an element coincident with side \p i. The element returned is
   * full-ordered, in contrast to the side method.  For example, calling
   * build_side(0) on a 20-noded hex will build a 8-noded quadrilateral
   * coincident with face 0 and pass back the pointer.
   *
   * A \p AutoPtr<Elem> is returned to prevent a memory leak.
   * This way the user need not remember to delete the object.
   *
   * The second argument, which is true by default, specifies that a
   * "proxy" element (of type Side) will be returned.  This type of
   * return value is useful because it does not allocate additional
   * memory, and is usually sufficient for FE calculation purposes.
   * If you really need a full-ordered, non-proxy side object, call
   * this function with proxy=false.
   */
  virtual AutoPtr<Elem> build_side (const unsigned int i,
                                    bool proxy=true) const = 0;

  /**
   * @return the ith node on sth side
   */
  virtual unsigned int side_node(unsigned int s, unsigned int i) const=0;

  /**
   * Creates an element coincident with edge \p i. The element returned is
   * full-ordered.  For example, calling build_edge(0) on a 20-noded hex will
   * build a 3-noded edge coincident with edge 0 and pass back the pointer.
   *
   * A \p AutoPtr<Elem> is returned to prevent a memory leak.
   * This way the user need not remember to delete the object.
   */
  virtual AutoPtr<Elem> build_edge (const unsigned int i) const = 0;

  /**
   * @returns the default approximation order for this element type.
   * This is the order that will be used to compute the map to the
   * reference element.
   */
  virtual Order default_order () const = 0;

  /**
   * @returns the centriod of the element. The centroid is
   * computed as the average of all the element vertices.
   * This method is overloadable since some derived elements
   * might want to use shortcuts to compute their centroid.
   */
  virtual Point centroid () const;

  /**
   * @returns the minimum vertex separation for the element.
   */
  virtual Real hmin () const;

  /**
   * @returns the maximum vertex separation for the element.
   */
  virtual Real hmax () const;

  /**
   * @return the gradient of input variable in the cell,
   * however we should never reach here
   */
  virtual VectorValue<PetscScalar> gradient( const std::vector<PetscScalar> & ) const
  { genius_error(); VectorValue<PetscScalar> dummy(0,0,0); return dummy; }

  /**
   * @return the gradient of input \p complex variable in the cell,
   * however we should never reach here
   */
  virtual VectorValue<Complex> gradient( const std::vector<Complex> & ) const
  { genius_error(); VectorValue<Complex> dummy(0,0,0); return dummy; }

  /**
   * @return the gradient of input \p AD variable in the cell,
   * however we should never reach here
   */
  virtual VectorValue<AutoDScalar> gradient( const std::vector<AutoDScalar> & ) const
  { genius_error(); VectorValue<AutoDScalar> dummy(0,0,0); return dummy; }

  /**
   * when we know the projection of vector V to each edge of the cell, use least-squares method to
   * reconstruct vector V
   * however we should never reach here
   */
  virtual VectorValue<PetscScalar> reconstruct_vector( const std::vector<PetscScalar> & ) const
  { genius_error(); VectorValue<PetscScalar> dummy(0,0,0); return dummy; }

  /**
   * when we know the projection of vector V to each edge of the cell, use least-squares method to
   * reconstruct vector V
   * however we should never reach here
   */
  virtual VectorValue<AutoDScalar> reconstruct_vector( const std::vector<AutoDScalar> & ) const
  { genius_error(); VectorValue<AutoDScalar> dummy(0,0,0); return dummy; }

  /**
   * @return the interpolated value at given point
   */
  virtual PetscScalar interpolation( const std::vector<PetscScalar> & , const Point &) const;

  /**
   * @return the (length/area/volume) of the geometric element.
   * however, we should never get here
   */
  virtual Real volume () const {genius_error(); return 0;}

  /**
   * @return the length of the ith edge of element.
   * however, we should never get here
   */
  virtual Real edge_length(const unsigned int i) const {genius_error(); return 0;}

  /**
   * @return the node associated partial (length/area/volume) of the geometric element with local node index.
   * only functional for FVM elements. return 0 for FEM elements
   */
  virtual Real partial_volume (unsigned int i) const
  {genius_assert( i<n_nodes() ); return 0;}

  /**
   * @return the edge associated partial (length/area) of the geometric element with local edge index.
   * only for 2D/3D however, we should never get here
   */
  virtual Real partial_area_with_edge(unsigned int e) const
  {genius_assert( e<n_edges() ); return 0;}

  /**
   * @return the edge associated partial (area/volume) of the geometric element with local edge index.
   * only for 2D/3D however, we should never get here
   */
  virtual Real partial_volume_with_edge(unsigned int e) const
  {genius_assert( e<n_edges() ); return 0;}


  /**
   * @return the edge associated truncated partial (length/area) of the geometric element with local edge index.
   * truncated means this value should be positive, some element has their own specific treatment
   */
  virtual Real partial_area_with_edge_truncated(unsigned int e) const
  { return std::max(0.0, this->partial_area_with_edge(e)); }

  /**
   * @return the edge associated truncated partial (area/volume) of the geometric element with local edge index.
   * truncated means this value should be positive, some element has their own specific treatment
   */
  virtual Real partial_volume_with_edge_truncated(unsigned int e) const
  { return std::max(0.0, this->partial_volume_with_edge(e)); }


  /**
   * Based on the quality metric q specified by the user,
   * returns a quantitative assessment of element quality.
   */
  virtual Real quality (const ElemQuality q) const;

  /**
   * Returns the suggested quality bounds for
   * the hex based on quality measure q.  These are
   * the values suggested by the CUBIT User's Manual.
   * Since this function can have no possible meaning
   * for an abstract Elem, it is an error.
   */
  virtual std::pair<Real,Real> qual_bounds (const ElemQuality) const
  { genius_error(); return std::make_pair(0.,0.); }

  /**
   * @returns true if the point p is contained in this element,
   * false otherwise.
   */
  virtual bool contains_point (const Point& p) const;

   /**
    * get the ray elem intersection result
    * @param p   start point of the ray
    * @param dir direction of the ray
    * @param result intersection result
    * @param dim    do 2d/3d intersection test
    */
   virtual void ray_hit(const Point & /* p */ , const Point & /* dir */ , IntersectionResult & /* result */ , unsigned int=3 /* dim */ ) const
   {genius_error(); return;}

   /**
    * @return the nearest point on this element to the given point p
    */
   virtual Point nearest_point(const Point &p, Real * dist = 0) const=0;

   /**
    * @returns the unit normal vector of a specified side for any element. The return value
    * in the function is a point type, and the vector is the side's outside normal vector.
    */
   virtual Point outside_unit_normal(unsigned short int side_id) const;

   /**
    * get the side which ray's terminate point is on in an element, for special instance this
    * terminate point may be shared by 2 or more sides, we get these sides by a vector. The
    * purpose to save these sides is for determine whether this terminate point is on Neumann
    * boundary.
    */
   std::vector<unsigned short int> get_terminate_side(const Point & terminate_point) const;

  /**
   * @returns true iff the element map is definitely affine (i.e. the same at
   * every quadrature point) within numerical tolerances
   */
  virtual bool has_affine_map () const { return false; }

  /**
   * @returns \p true if the element is active (i.e. has no active
   * descendants), \p false  otherwise. Note that it suffices to check the
   * first child only. Always returns \p true if AMR is disabled.
   */
  bool active () const;

  /**
   * @returns \p true if the element is an ancestor (i.e. has an
   * active child or ancestor child), \p false otconst herwise. Always
   * returns \p false if AMR is disabled.
   */
  bool ancestor () const;

  /**
   * @returns \p true if the element is subactive (i.e. has no active
   * descendants), \p false otherwise. Always returns \p false if AMR
   * is disabled.
   */
  bool subactive () const;

  /**
   * @returns \p true if the element has any children (active or not),
   * \p false  otherwise. Always returns \p false if AMR is disabled.
   */
  bool has_children () const;

  /**
   * @returns the actual number of children this element has
   */
  unsigned int has_n_children () const;

  /**
   * @returns \p true if the element has any descendants other than
   * its immediate children, \p false otherwise. Always returns \p
   * false if AMR is disabled.
   */
  bool has_ancestor_children () const;

  /**
   * @returns \p true if \p descendant is a child of \p this, or a
   * child of a child of \p this, etc.
   * Always returns \p false if AMR is disabled.
   */
  bool is_ancestor_of(const Elem *descendant) const;

  /**
   * @returns a const pointer to the element's parent.  Returns \p NULL if
   * the element was not created via refinement, i.e. was read from file.
   */
  const Elem* parent () const;

  /**
   * @returns a pointer to the element's parent.  Returns \p NULL if
   * the element was not created via refinement, i.e. was read from file.
   */
  Elem* parent ();

  /**
   * @sets the pointer to the element's parent.
   * Dangerous to use in high-level code.
   */
  void set_parent (Elem *p);

  /**
   * @returns a pointer to the element's top-most (i.e. level-0) parent.
   * Returns \p this if this is a level-0 element, this element's parent
   * if this is a level-1 element, this element's grandparent if this is
   * a level-2 element, etc...
   */
  const Elem* top_parent () const;

  /**
   * @returns the magnitude of the distance between nodes n1 and n2.
   * Useful for computing the lengths of the sides of elements.
   */
  Real length (const unsigned int n1,
               const unsigned int n2) const;

  /**
   * @returns the number of adjacent vertices, that uniquely define
   * the location of the \f$ n^{th} \f$ @e second-order node.  For linear
   * elements ( \p default_order()==FIRST ), this returns 0.
   * This method is useful when converting linear elements to quadratic
   * elements.  Note that \p n has to be greater or equal
   * \p this->n_vertices().
   */
  virtual unsigned int n_second_order_adjacent_vertices (const unsigned int n) const;

  /**
   * @returns the element-local number of the  \f$ v^{th} \f$ vertex
   * that defines the \f$ n^{th} \f$ second-order node.  Note that
   * the return value is always less \p this->n_vertices(), while
   * \p n has to be greater or equal \p this->n_vertices().  For
   * linear elements this returns 0.
   */
  virtual unsigned short int second_order_adjacent_vertex (const unsigned int n,
                                                           const unsigned int v) const;

  /**
   * @returns the child number \p c and element-local index \p v of the
   * \f$ n^{th} \f$ second-order node on the parent element.  Note that
   * the return values are always less \p this->n_children() and
   * \p this->child(c)->n_vertices(), while \p n has to be greater or equal
   * to \p * this->n_vertices().  For linear elements this returns 0,0.
   * On refined second order elements, the return value will satisfy
   * \p this->get_node(n)==this->child(c)->get_node(v)
   */
  virtual std::pair<unsigned short int, unsigned short int>
          second_order_child_vertex (const unsigned int n) const;

  /**
   * @returns the element type of the associated second-order element,
   * e.g. when \p this is a \p TET4, then \p TET10 is returned.  Returns
   * \p INVALID_ELEM for second order or other elements that should not
   * or cannot be converted into higher order equivalents.
   *
   * For some elements, there exist two second-order equivalents, e.g.
   * for \p Quad4 there is \p Quad8 and \p Quad9.  When the optional
   * \p full_ordered is \p true, then \p QUAD9 is returned.  When
   * \p full_ordered is \p false, then \p QUAD8 is returned.
   */
  static ElemType second_order_equivalent_type (const ElemType et,
                                                const bool full_ordered=true);

  /**
   * @returns the element type of the associated first-order element,
   * e.g. when \p this is a \p TET10, then \p TET4 is returned.  Returns
   * \p INVALID_ELEM for first order or other elements that should not
   * or cannot be converted into lower order equivalents.
   */
  static ElemType first_order_equivalent_type (const ElemType et);

  /**
   * @returns the element type which may be use in FVM.
   * i.e. TET4, PRISM6, HEX8.
   * However, the user should do more test (in sphere and co-plane test) to make sure
   * the elem is FVM compatible
   */
  static ElemType fvm_compatible_type (const ElemType et);

  /**
   * @returns dimension by elem type
   */
  static unsigned int dim (const ElemType et);

  /**
   * return \p true if the element can be used in FVM
   */
  virtual bool fvm_compatible_test() const
  { return false; }


  /**
   * calculate geom information for fvm usage, default does nothing
   */
  virtual void prepare_for_fvm() {}


  /**
   * @returns the refinement level of the current element.  If the
   * element's parent is \p NULL then by convention it is at
   * level 0, otherwise it is simply at one level greater than
   * its parent.
   */
  unsigned int level () const;

  /**
   * Returns the value of the p refinement level of an active
   * element, or the minimum value of the p refinement levels
   * of an ancestor element's descendants
   */
  unsigned int p_level () const;

#ifdef ENABLE_AMR

  /**
   * Useful ENUM describing the refinement state of
   * an element.
   */
  enum RefinementState { COARSEN = 0,
                         DO_NOTHING,
                         REFINE,
                         JUST_REFINED,
                         JUST_COARSENED,
                         INACTIVE,
                         COARSEN_INACTIVE };

  /**
   * @returns a pointer to the \f$ i^{th} \f$ child for this element.
   * Do not call if this element has no children, i.e. is active.
   */
  Elem* child (const unsigned int i) const;

  /**
   * This function tells you which child you \p (e) are.
   * I.e. if c = a->which_child_am_i(e); then
   * a->child(c) will be e;
   */
  unsigned int which_child_am_i(const Elem *e) const;

  /**
   * @returns true iff the specified child is on the
   * specified side
   */
  virtual bool is_child_on_side(const unsigned int c,
                                const unsigned int s) const=0;

  /**
   * @returns true iff the specified child is on the
   * specified side
   */
  bool is_child_on_side(const Elem * child,
                        const unsigned int s) const;

  /**
   * @returns true iff the specified child is on the
   * specified edge
   */
  virtual bool is_child_on_edge(const unsigned int c,
                                const unsigned int e) const;

  /**
   * @returns true iff the specified child is on the
   * specified edge
   */
  virtual bool is_child_on_edge(const Elem * child,
                                const unsigned int e) const;

  /**
   * Adds a child pointer to the array of children of this element.
   * If this is the first child to be added, this method allocates
   * memory in the parent's _children array, otherwise, it just sets
   * the pointer.
   */
  void add_child (Elem* elem);

  /**
   * Adds a child pointer to the array of children of this element.
   * If this is the first child to be added, this method allocates
   * memory in the parent's _children array, otherwise, it just sets
   * the pointer at \p pos.
   */
  void add_child (Elem* elem, unsigned int pos);

  /**
   * Delete a child pointer to the array of children of this element.
   * If parent's _children array is NULL, report an error.
   */
  void delete_child (Elem* elem);


  /**
   * Fills the vector \p family with the children of this element,
   * recursively.  So, calling this method on a twice-refined element
   * will give you the element itself, its direct children, and their
   * children, etc...  When the optional parameter \p reset is
   * true then the vector will be cleared before the element and its
   * descendants are added.
   */
  void family_tree (std::vector<const Elem*>& family,
                    const bool reset=true) const;

  /**
   * Same as the \p family_tree() member, but only adds the active
   * children.  Can be thought of as removing all the inactive
   * elements from the vector created by \p family_tree, but is
   * implemented more efficiently.
   */
  void active_family_tree (std::vector<const Elem*>& active_family,
                           const bool reset=true) const;

  /**
   * Same as the \p family_tree() member, but only adds elements
   * which are next to \p neighbor.
   */
  void family_tree_by_neighbor (std::vector<const Elem*>& family,
                                const Elem *neighbor,
                                const bool reset=true) const;

  /**
   * Same as the \p active_family_tree() member, but only adds elements
   * which are next to \p neighbor.
   */
  void active_family_tree_by_neighbor (std::vector<const Elem*>& family,
                                       const Elem *neighbor,
                                       const bool reset=true) const;


  /**
   * Same as the \p family_tree() member, but only adds elements
   * which are next to \p side.
   */
  void family_tree_by_side (std::vector<const Elem*>& family,
                            const unsigned int s,
                            const bool reset=true) const;

  /**
   * Same as the \p active_family_tree() member, but only adds elements
   * which are next to \p side.
   */
  void active_family_tree_by_side (std::vector<const Elem*>& family,
                                   const unsigned int s,
                                   const bool reset=true) const;

  /**
   * @return the hanging node on side s of this element, NULL for none hanging node
   */
  virtual const Node * is_hanging_node_on_side(unsigned int ) const
  { return NULL; }


  /**
   * @return the hanging node on edge e of this element, NULL for none hanging node
   */
  virtual const Node * is_hanging_node_on_edge(unsigned int ) const
  { return NULL; }


  /**
   * Returns the value of the refinement flag for the element.
   */
  RefinementState refinement_flag () const;

  /**
   * Sets the value of the refinement flag for the element.
   */
  void set_refinement_flag (const RefinementState rflag);

  /**
   * Returns the value of the p refinement flag for the element.
   */
  RefinementState p_refinement_flag () const;

  /**
   * Sets the value of the p refinement flag for the element.
   */
  void set_p_refinement_flag (const RefinementState pflag);

  /**
   * Returns the maximum value of the p refinement levels of
   * an ancestor element's descendants
   */
  unsigned int max_descendant_p_level () const;

  /**
   * Returns the minimum p refinement level of elements which
   * are descended from this and which share a side with the
   * active \p neighbor
   */
  unsigned int min_p_level_by_neighbor (const Elem* neighbor,
                                        unsigned int current_min) const;

  /**
   * Returns the minimum new p refinement level (i.e. after
   * refinement and coarsening is done) of elements which are
   * descended from this and which share a side with the
   * active \p neighbor
   */
  unsigned int min_new_p_level_by_neighbor (const Elem* neighbor,
                                            unsigned int current_min) const;

  /**
   * Sets the value of the p refinement level for the element
   * Note that the maximum p refinement level is currently 255
   */
  void set_p_level (const unsigned int p);

  /**
   * Sets the value of the p refinement level for the element
   * without altering the p level of its ancestors
   */
  void hack_p_level (const unsigned int p);

  /**
   * Refine the element.
   */
  virtual void refine (MeshRefinement& mesh_refinement);

  /**
   * Coarsen the element.  This is not
   * virtual since it is the same for all
   * element types.
   */
  void coarsen ();

  /**
   * Contract an active element, i.e. remove pointers to any
   * subactive children.  This should only be called via
   * MeshRefinement::contract, which will also remove subactive
   * children from the mesh
   */
  void contract ();

  /**
   * Pack all this information into one communication to avoid latency
   */
  void pack_element (std::vector<int> &conn) const;

  /**
   * elem pack size
   */
  static unsigned int pack_size( ElemType t );

#endif


protected:
  /**
   * The protected nested SideIter class is used to iterate over the
   * sides of this Elem.  It is a specially designed class since
   * no sides are actually stored by the element.  This iterator-like
   * class has to provide the following three operations
   * 1) operator*
   * 2) operator++
   * 3) operator==
   * The definition can be found at the end of this header file.
   */
  class SideIter;

public:
  /**
   * Useful iterator typedefs
   */
  typedef Predicates::multi_predicate Predicate;
  //typedef variant_filter_iterator<Elem*, Predicate> side_iterator;

  /**
   * Data structure for iterating over sides.  Defined at the end of
   * this header file.
   */
  struct side_iterator;

  /**
   * Iterator accessor functions
   */
  side_iterator boundary_sides_begin();
  side_iterator boundary_sides_end();

private:
  /**
   * Side iterator helper functions.  Used to replace the begin()
   * and end() functions of the STL containers.
   */
  SideIter _first_side();
  SideIter _last_side();

public:

#ifdef ENABLE_INFINITE_ELEMENTS

  /**
   * @returns \p true if the element is an infinite element,
   * \p false  otherwise.
   */
  virtual bool infinite () const = 0;

  /**
   * @returns the origin for an infinite element.  Currently,
   * @e all infinite elements used in a mesh share the same
   * origin.  Overload this in infinite element classes.
   * By default, issues an error, because returning the
   * all zero point would very likely lead to unexpected
   * behavior.
   */
  virtual Point origin () const { genius_error(); return Point(); }

#endif




  /**
   * Build an element of type \p type.  Since this method
   * allocates memory the new \p Elem is returned in a
   * \p AutoPtr<>
   */
  static AutoPtr<Elem> build (const ElemType type,
                              Elem* p=NULL);


 /**
  * Build an clone element of type \p type. which has its own nodes
  * Since this method allocates memory the new \p Elem is returned
  * in a \p AutoPtr<>
  */
  static AutoPtr<Elem> build_clone (const ElemType type,
                                    Elem* p=NULL);

#ifdef ENABLE_AMR

  /**
   * Matrix that transforms the parents nodes into the children's
   * nodes
   */
  virtual float embedding_matrix (const unsigned int i,
                                  const unsigned int j,
                                  const unsigned int k) const = 0;

#endif



 protected:

  //-------------------------------------------------------
  // These methods compute has keys from the specified
  // global node numbers
  //
  /**
   * Compute a key from the specified nodes.
   */
  static unsigned int compute_key (unsigned int n0);

  /**
   * Compute a key from the specified nodes.
   */
  static unsigned int compute_key (unsigned int n0,
                                   unsigned int n1);

  /**
   * Compute a key from the specified nodes.
   */
  static unsigned int compute_key (unsigned int n0,
                                   unsigned int n1,
                                   unsigned int n2);

  /**
   * Compute a key from the specified nodes.
   */
  static unsigned int compute_key (unsigned int n0,
                                   unsigned int n1,
                                   unsigned int n2,
                                   unsigned int n3);
  //-------------------------------------------------------



  /**
   * Replaces this element with \p NULL for all of
   * its neighbors.  This is useful when deleting an
   * element.
   */
  void nullify_neighbors ();

  /**
   * Pointers to the nodes we are conneted to.
   */
  Node** _nodes;

  /**
   * Pointers to this element's neighbors.
   */
  Elem** _neighbors;

  /**
   * A pointer to this element's parent.
   */
  Elem* _parent;

#ifdef ENABLE_AMR

  /**
   * Pointers to this element's children.
   */
  Elem** _children;

  /**
   * h refinement flag. This is stored as an unsigned char
   * to save space.
   */
  unsigned char _rflag;
  //RefinementState _rflag;

  /**
   * p refinement flag. This is stored as an unsigned char
   * to save space.
   */
  unsigned char _pflag;
  //RefinementState _pflag;

  /**
   * p refinement level - the difference between the
   * polynomial degree on this element and the minimum
   * polynomial degree on the mesh.
   * This is stored as an unsigned char to save space.
   * In theory, these last four bytes might have
   * been padding anyway.
   */
  unsigned char _p_level;

#endif

  /**
   * The subdomain to which this element belongs.
   */
  unsigned int _sbd_id;

  /**
   * Make the classes that need to access our build
   * member friends.  These classes do not really fit
   * the profile of what a "friend" should be, but
   * if we are going to protect the constructor and
   * the build method, there's no way around it.
   *
   * Do we *really* need to protect the build member?
   * It would seem that we are just getting around it
   * by using friends!
   */
  friend class MeshRefinement;    // (Elem::nullify_neighbors)

 private:

  // Prime numbers used for computing node keys.  These are the same
  // for every instance of the Elem class.
  static const unsigned int _bp1;
  static const unsigned int _bp2;
};




// ------------------------------------------------------------
// Elem class member functions
inline
Elem::Elem(const unsigned int nn,
           const unsigned int ns,
           Elem* p) :
  _parent(p)
#ifdef ENABLE_AMR
  , _p_level(0)
#endif
{
  this->subdomain_id() = 0;
  this->processor_id() = 0;

  // Initialize the nodes data structure
  _nodes = NULL;

  if (nn != 0)
    {
      _nodes = new Node*[nn];

      for (unsigned int n=0; n<nn; n++)
        _nodes[n] = NULL;
    }

  // Initialize the neighbors data structure
  _neighbors = NULL;

  if (ns != 0)
    {
      _neighbors = new Elem*[ns];

      for (unsigned int n=0; n<ns; n++)
        _neighbors[n] = NULL;
    }

  // Optionally initialize data from the parent
  if (this->parent() != NULL)
    {
      this->subdomain_id() = this->parent()->subdomain_id();
      this->processor_id() = this->parent()->processor_id();
    }

#ifdef ENABLE_AMR

  _children = NULL;

  this->set_refinement_flag(Elem::DO_NOTHING);

  this->set_p_refinement_flag(Elem::DO_NOTHING);

  if (this->parent())
    this->set_p_level(parent()->p_level());
  else
    this->set_p_level(0);

#endif
}



inline
Elem::~Elem()
{
  // Delete my node storage
  if (_nodes != NULL)
    delete [] _nodes;
  _nodes = NULL;

  // Delete my neighbor storage
  if (_neighbors != NULL)
    delete [] _neighbors;
  _neighbors = NULL;

#ifdef ENABLE_AMR

  // Delete my children's storage
  if (_children != NULL)
    delete [] _children;
  _children = NULL;

#endif
}



inline
const Point & Elem::point (const unsigned int i) const
{
  assert (i < this->n_nodes());
  assert (_nodes[i] != NULL);
  assert (_nodes[i]->id() != Node::invalid_id);

  return *_nodes[i];
}



inline
Point & Elem::point (const unsigned int i)
{
  assert (i < this->n_nodes());

  return *_nodes[i];
}



inline
unsigned int Elem::node (const unsigned int i) const
{
  assert (i < this->n_nodes());
  assert (_nodes[i] != NULL);
  assert (_nodes[i]->id() != Node::invalid_id);

  return _nodes[i]->id();
}



inline
Node* Elem::get_node (const unsigned int i) const
{
  assert (i < this->n_nodes());
  assert (_nodes[i] != NULL);

  return _nodes[i];
}



inline
Node* & Elem::set_node (const unsigned int i)
{
  assert (i < this->n_nodes());

  return _nodes[i];
}



inline
unsigned int Elem::subdomain_id () const
{
  return _sbd_id;
}


inline
unsigned int Elem::subdomain_id (const unsigned int i) const
{
  if(!_neighbors[i]) return invalid_uint;
  return _neighbors[i]->subdomain_id ();
}


inline
unsigned int & Elem::subdomain_id ()
{
  return _sbd_id;
}



inline
Elem* Elem::neighbor (const unsigned int i) const
{
  assert (i < this->n_neighbors());

  return _neighbors[i];
}



inline
void Elem::set_neighbor (const unsigned int i, Elem* n)
{
  assert (i < this->n_neighbors());

  _neighbors[i] = n;
}



inline
bool Elem::is_neighbor (const Elem* elem) const
{
  for (unsigned int n=0; n<this->n_neighbors(); n++)
    if (this->neighbor(n) == elem)
      return true;

  return false;
}



inline
Elem* Elem::child_neighbor (Elem* elem) const
{
  for (unsigned int n=0; n<elem->n_neighbors(); n++)
    if (elem->neighbor(n) &&
        elem->neighbor(n)->parent() == this)
      return elem->neighbor(n);

  return NULL;
}



inline
const Elem* Elem::child_neighbor (const Elem* elem) const
{
  for (unsigned int n=0; n<elem->n_neighbors(); n++)
    if (elem->neighbor(n) &&
        elem->neighbor(n)->parent() == this)
      return elem->neighbor(n);

  return NULL;
}



inline
bool Elem::on_boundary () const
{
  // By convention, the element is on the boundary
  // if it has a NULL neighbor.
  return this->is_neighbor(NULL);
}



inline
bool Elem::on_boundary (const unsigned int s) const
{
  // By convention, the element is on the boundary
  // if it has a NULL neighbor.
  return this->neighbor(s) == NULL;
}


inline
bool Elem::on_interface () const
{
  genius_assert(this->n_neighbors()>0);

  for (unsigned int n=0; n<this->n_neighbors(); n++)
  {
    if( this->neighbor(n) == NULL ) continue;
    if (this->neighbor(n)->subdomain_id() != this->subdomain_id())
      return true;
  }
  return false;

}



inline
bool Elem::on_interface (const unsigned int s) const
{
  genius_assert(this->n_neighbors()>0);

  if( this->neighbor(s) == NULL ) return false;

  if (this->neighbor(s)->subdomain_id() != this->subdomain_id())
      return true;

  return false;

}



inline
unsigned int Elem::which_neighbor_am_i (const Elem* e) const
{
  assert (e != NULL);

  const Elem* eparent = e;

  while (eparent->level() > this->level())
    eparent = eparent->parent();

  for (unsigned int s=0; s<this->n_neighbors(); s++)
    if (this->neighbor(s) == eparent)
      return s;


  std::cerr << "ERROR:  Elements are not neighbors!"
            << std::endl;

  genius_error();

  return invalid_uint;
}



inline
bool Elem::active() const
{
#ifdef ENABLE_AMR
  if ((this->refinement_flag() == INACTIVE) ||
      (this->refinement_flag() == COARSEN_INACTIVE))
    return false;
  else
    return true;
#else
  return true;
#endif
}





inline
bool Elem::subactive() const
{
#ifdef ENABLE_AMR
  if (this->active())
    return false;
  if (!this->has_children())
    return true;
  return this->child(0)->subactive();
#else
  return false;
#endif
}



inline
bool Elem::has_children() const
{
#ifdef ENABLE_AMR
  if (_children == NULL)
    return false;
  else
    return true;
#else
  return false;
#endif
}


inline
unsigned int Elem::has_n_children () const
{
#ifdef ENABLE_AMR
  if (_children == NULL)
    return 0;
  else
  {
    unsigned int n_children = 0;

    for (unsigned int c=0; c != this->n_children(); c++)
      if ( _children[c] != NULL )
        ++n_children;

    return n_children;
  }
#else
  return 0;
#endif

}



inline
bool Elem::has_ancestor_children() const
{
#ifdef ENABLE_AMR
  if (_children == NULL)
    return false;
  else
    for (unsigned int c=0; c != this->n_children(); c++)
      if (this->child(c)->has_children())
        return true;
#endif
  return false;
}



#ifdef ENABLE_AMR
inline
bool Elem::is_ancestor_of(const Elem *descendant) const
{
  const Elem *e = descendant;
  while (e)
    {
      if (this == e)
        return true;
      e = e->parent();
    }
#else
inline
bool Elem::is_ancestor_of(const Elem *) const
{
#endif
  return false;
}


inline
const Elem* Elem::parent () const
{
  return _parent;
}



inline
Elem* Elem::parent ()
{
  return _parent;
}



inline
void Elem::set_parent (Elem *p)
{
  _parent = p;
}



inline
const Elem* Elem::top_parent () const
{
  const Elem* tp = this;

  // Keep getting the element's parent
  // until that parent is at level-0
  while (tp->parent() != NULL)
    tp = tp->parent();

  assert (tp != NULL);
  assert (tp->level() == 0);

  return tp;
}



inline
unsigned int Elem::level() const
{
#ifdef ENABLE_AMR

  // if I don't have a parent I was
  // created directly from file
  // or by the user, so I am a
  // level-0 element
  if (this->parent() == NULL)
    return 0;

  // otherwise we are at a level one
  // higher than our parent
  return (this->parent()->level() + 1);

#else

  // Without AMR all elements are
  // at level 0.
  return 0;

#endif
}



inline
unsigned int Elem::p_level() const
{
#ifdef ENABLE_AMR
  return _p_level;
#else
  return 0;
#endif
}



#ifdef ENABLE_AMR

inline
Elem* Elem::child (const unsigned int i) const
{
  assert (_children    != NULL);
  assert (_children[i] != NULL);

  return _children[i];
}



inline
unsigned int Elem::which_child_am_i (const Elem* e) const
{
  assert (e != NULL);
  assert (this->has_children());

  for (unsigned int c=0; c<this->n_children(); c++)
    if (this->child(c) == e)
      return c;

  std::cerr << "ERROR:  which_child_am_i() was called with a non-child!"
            << std::endl;

  genius_error();

  return invalid_uint;
}



inline
Elem::RefinementState Elem::refinement_flag () const
{
  return static_cast<RefinementState>(_rflag);
}



inline
void Elem::set_refinement_flag(RefinementState rflag)
{
#ifdef DEBUG
  if (rflag != static_cast<RefinementState>(static_cast<unsigned char>(rflag)))
    {
      std::cerr << "ERROR: unsigned char too small to hold Elem::_rflag!"
                << std::endl
                << "Recompile with Elem:_*flag set to something bigger!"
                << std::endl;
      genius_error();
    }
#endif

  _rflag = rflag;
}



inline
Elem::RefinementState Elem::p_refinement_flag () const
{
  return static_cast<RefinementState>(_pflag);
}



inline
void Elem::set_p_refinement_flag(RefinementState pflag)
{
#ifdef DEBUG
  if (pflag != static_cast<RefinementState>(static_cast<unsigned char>(pflag)))
    {
      std::cerr << "ERROR: unsigned char too small to hold Elem::_pflag!"
                << std::endl
                << "Recompile with Elem:_*flag set to something bigger!"
                << std::endl;
      genius_error();
    }
#endif

  _pflag = pflag;
}



inline
unsigned int Elem::max_descendant_p_level () const
{
  // This is undefined for subactive elements,
  // which have no active descendants
  assert (!this->subactive());
  if (this->active())
    return this->p_level();

  unsigned int max_p_level = _p_level;
  for (unsigned int c=0; c != this->n_children(); c++)
    max_p_level = std::max(max_p_level,
                           this->child(c)->max_descendant_p_level());
  return max_p_level;
}



inline
void Elem::set_p_level(unsigned int p)
{
#ifdef DEBUG
  if (p != static_cast<unsigned int>(static_cast<unsigned char>(p)))
    {
      std::cerr << "ERROR: unsigned char too small to hold Elem::_p_level!"
                << std::endl
                << "Recompile with Elem:_p_level set to something bigger!"
                << std::endl;
      genius_error();
    }
#endif

  // Maintain the parent's p level as the minimum of it's children
  if (this->parent() != NULL)
    {
      unsigned int parent_p_level = this->parent()->p_level();

      // If our new p level is less than our parents, our parents drops
      if (parent_p_level > p)
        {
          this->parent()->set_p_level(p);
        }
      // If we are the lowest p level and it increases, so might
      // our parent's, but we have to check every other child to see
      else if (parent_p_level == _p_level && _p_level < p)
        {
          _p_level = p;
          parent_p_level = p;
          for (unsigned int c=0; c != this->parent()->n_children(); c++)
            parent_p_level = std::min(parent_p_level,
                                      this->parent()->child(c)->p_level());

          if (parent_p_level != this->parent()->p_level())
            this->parent()->set_p_level(parent_p_level);

          return;
        }
    }

  _p_level = p;
}



inline
void Elem::hack_p_level(unsigned int p)
{
  _p_level = p;
}



#endif /* ifdef ENABLE_AMR */


inline
unsigned int Elem::compute_key (unsigned int n0)
{
  return n0;
}



inline
unsigned int Elem::compute_key (unsigned int n0,
                                unsigned int n1)
{
  // big prime number
  const unsigned int bp = 65449;

  // Order the two so that n0 < n1
  if (n0 > n1) std::swap (n0, n1);

  return (n0%bp + (n1<<5)%bp);
}



inline
unsigned int Elem::compute_key (unsigned int n0,
                                unsigned int n1,
                                unsigned int n2)
{
  // big prime number
  const unsigned int bp = 65449;

  // Order the numbers such that n0 < n1 < n2.
  // We'll do it in 3 steps like this:
  //
  //     n0         n1                n2
  //     min(n0,n1) max(n0,n1)        n2
  //     min(n0,n1) min(n2,max(n0,n1) max(n2,max(n0,n1)
  //           |\   /|                  |
  //           | \ / |                  |
  //           |  /  |                  |
  //           | /  \|                  |
  //  gb min= min   max              gb max



  // Step 1
  if (n0 > n1) std::swap (n0, n1);

  // Step 2
  if (n1 > n2) std::swap (n1, n2);

  // Step 3
  if (n0 > n1) std::swap (n0, n1);

  assert ((n0 < n1) && (n1 < n2));


  return (n0%bp + (n1<<5)%bp + (n2<<10)%bp);
}



inline
unsigned int Elem::compute_key (unsigned int n0,
                                unsigned int n1,
                                unsigned int n2,
                                unsigned int n3)
{
  // big prime number
  const unsigned int bp = 65449;

  // Step 1
  if (n0 > n1) std::swap (n0, n1);

  // Step 2
  if (n2 > n3) std::swap (n2, n3);

  // Step 3
  if (n0 > n2) std::swap (n0, n2);

  // Step 4
  if (n1 > n3) std::swap (n1, n3);

  // Finally step 5
  if (n1 > n2) std::swap (n1, n2);

  assert ((n0 < n1) && (n1 < n2) && (n2 < n3));

  return (n0%bp + (n1<<5)%bp + (n2<<10)%bp + (n3<<15)%bp);
}




/**
 * The definition of the protected nested SideIter class.
 */
class Elem::SideIter
{
public:
  // Constructor with arguments.
  SideIter(const unsigned int side_number,
           Elem* parent)
    : _side_number(side_number),
      _side_ptr(NULL),
      _parent(parent)
  {}


  // Empty constructor.
  SideIter()
    : _side_number(invalid_uint),
      _side_ptr(NULL),
      _parent(NULL)
  {}


  // Copy constructor
  SideIter(const SideIter& other)
    : _side_number(other._side_number),
      _parent(other._parent)
  {}


  // op=
  SideIter& operator=(const SideIter& other)
  {
    this->_side_number = other._side_number;
    this->_parent      = other._parent;
    return *this;
  }

  // unary op*
  Elem*& operator*() const
  {
    // Set the AutoPtr
    this->_update_side_ptr();

    // Return a reference to _side_ptr
    return this->_side_ptr;
  }

  // op++
  SideIter& operator++()
  {
    ++_side_number;
    return *this;
  }

  // op==  Two side iterators are equal if they have
  // the same side number and the same parent element.
  bool operator == (const SideIter& other) const
  {
    return (this->_side_number == other._side_number &&
            this->_parent      == other._parent);
  }


  // Consults the parent Elem to determine if the side
  // is a boundary side.  Note: currently side N is a
  // boundary side if nieghbor N is NULL.  Be careful,
  // this could possibly change in the future?
  bool side_on_boundary() const
  {
    return this->_parent->neighbor(_side_number) == NULL;
  }

private:
  // Update the _side pointer by building the correct side.
  // This has to be called before dereferencing.
  void _update_side_ptr() const
  {
    // Construct new side, store in AutoPtr
    this->_side = this->_parent->build_side(this->_side_number);

    // Also set our internal naked pointer.  Memory is still owned
    // by the AutoPtr.
    this->_side_ptr = _side.get();
  }

  // A counter variable which keeps track of the side number
  unsigned int _side_number;

  // AutoPtr to the actual side, handles memory management for
  // the sides which are created during the course of iteration.
  mutable AutoPtr<Elem> _side;

  // Raw pointer needed to facilitate passing back to the user a
  // reference to a non-temporary raw pointer in order to conform to
  // the variant_filter_iterator interface.  It points to the same
  // thing the AutoPtr "_side" above holds.  What happens if the user
  // calls delete on the pointer passed back?  Well, this is an issue
  // which is not addressed by the iterators in libMesh.  Basically it
  // is a bad idea to ever call delete on an iterator from the library.
  mutable Elem* _side_ptr;

  // Pointer to the parent Elem class which generated this iterator
  Elem* _parent;

};






// Private implementation functions in the Elem class for the side iterators.
// They have to come after the definition of the SideIter class.
inline
Elem::SideIter Elem::_first_side()
{
  return SideIter(0, this);
}



inline
Elem::SideIter Elem::_last_side()
{
  return SideIter(this->n_neighbors(), this);
}




/**
 * The definition of the struct used for iterating over sides.
 */
struct
Elem::side_iterator :
variant_filter_iterator<Elem::Predicate,
                        Elem*>
{
  // Templated forwarding ctor -- forwards to appropriate variant_filter_iterator ctor
  template <typename PredType, typename IterType>
  side_iterator (const IterType& d,
                 const IterType& e,
                 const PredType& p ) :
    variant_filter_iterator<Elem::Predicate,
                            Elem*>(d,e,p) {}
};




#endif // end #ifndef __elem_h__
