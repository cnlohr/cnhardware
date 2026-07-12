/* cnrbtree.h - v0.2 - public domain C templated Red-Black Tree -
       https://github.com/cnlohr/cntools/cnrbtree
       no warranty implied; use at your own risk.

  If public domain is not possible, you may freely license this under the
  Unlicense, MIT, or any of the BSD licenses.

  This is effectively a data structure which provides C functionality similar
    to the C++ STL's <map>, <multimap> and <set>.  It is templated and
    supports anything that is comparable for the key, strings, ints, pointers,
    etc.

  Do this:

     #define CNRBTREE_IMPLEMENTATION

  Before you include this file in *one* C or C++ file to create the instance
  I.e. it should look something like this:

     #include <...>
     #include <...>
     #define CNRBTREE_IMPLEMENTATION
     #include "cnrbtree.h"

  Alternatively, if you are using a compile that supports the __weak__
  attribute you may just set CNRBTREE_GENERIC_DECORATOR and
  CNRBTREE_TEMPLATE_DECORATOR to __attribute__((weak)).  The default behavior
  is for CNRBTREE_TEMPLATE_DECORATOR to be static inline.  You can override
  it to be default visibility with
    #define CNRBTREE_GENERIC_DECORATOR __attribute__((weak))

  CNRBTREE_GENERIC_DECORATOR defults to __attribute__((noinline)) and is used
  on common functions which it seems many compilers are mislead into thinking
  they should inline.  You can override this behavior by
    #define CNRBTREE_GENERIC_DECORATOR
  

  This is templated red-black tree for C.  It can be used for storing things
  like dictionaries, iterating through automatically sorting lists, finding
  elements in a dictionary that are equal to or greater than, or other similar
  unusual operations.

  Because this is templated, which means that for every combination of types,
  you can create the template which builds the functions  and types customized
  for your specific type.  This allows for a number of rather positive
  features:
    * The payload lives inside the nodes which are holding it.
      * Less malloc/free
      * Better cache coherency.
    * Avoid function calls for comparisons (great for primitive types).
    * Syntax for using the general accessor is very nice as it is automatically
      typed for your specific types.
    * Allows the compiler to apply extra optimization to your specific type.

  Usage: Somewhere in your program, before using a type, define the template:

     typedef struct object_t
     {
       int myvalue;
     } object;
     typedef char * str;
     CNRBTREETEMPLATE( str, object, RBstrcmp, RBstrcpy, RBstrdel );

  This will define a tree which uses strings to index.  You can then create
  these types.  I.e.

     //Constructs the tree
     cnrbtree_strobject * tree = cnrbtree_strobject_create();

     //Accesses, like C++ map's [] operator.
     RBA( tree, "a" ).myvalue = 5;
     RBA( tree, "d" ).myvalue = 8;
     RBA( tree, "c" ).myvalue = 7;
     RBA( tree, "b" ).myvalue = 6;

     //Access, like [] but reading.
     printf( "%d\n", RBA(tree, "c").myvalue );

     //Iterate through them all.
     RBFOREACH( strobject, tree, i )
     {
         printf( ".key = %s .myvalue = %d\n", i->key, i->data.myvalue );
     }

     //Typesafe delete.
     RBDESTROY( tree );

  You can also just do strint, and it will use strings as the index, and
     integers as the payload.

  You can also treat this not like a c++ map, but a c++ multimap.  Normally
      the comparison function returns 0 if there is a match, if instead you
      always return -1 or +1, then, it will continue to insert items into
      the map with the identical key, for instance you can use this to
      create a priority queue, or to make a sorting system. You can do this
      with ints or ptrs with RBptrcmpnomatch.

  Also provided: cnptrset - for a set-type data structure.  It is actually
     a cnrbtree_rbset_trbset_null_t and can be used as such.  This looks odd
     but, the size of the payload is 0 and because the types are templated
     the actual overhead for having a 0 payload is zero.

     cnptrset * set = cnptrset_create();
     static int var;
     cnptrset_insert( set, &var );
     void * i; //Quirk in cnptrset_foreach. (NOTE you may want to use RBFOREACH)
     cnptrset_foreach( set, i ) { printf( "%p\n", i ); }
     cnptrset_remove( set, &var );
     cnptrset_destroy( set );

  Note that if your payload is a POD, like an int, it will start uninitialized,
     so you will have to check if it has been initialized yet with RBHAS.

     if( RBHAS( mytree, mykey ) )
        RBA( mytree, mykey )++;
     else
        RBA( mytree, mykey ) = 1;

  Authors:
    2019-2025 <>< Charles Lohr
 
  Based on Wikipedia article on red black trees.

  For judistictions where a public domain license is not available, the code
       may be licensed under:
   * New BSD (3-Clause) License
   * CC0 License
   * MIT/x11 License

  Version History:
     0.1pre - Initial Release (Incomplete and relatively slow)


  Various design notes:

    MIT Press Introduction to Algorithms version, modified to use uncles, also
      a bit rearranged as an optimization.  Does not use recursion. Basically
      from CLRS 3rd Edition; It's based off of Cormen's algorithm.
    https://dpb.bitbucket.io/annotations-of-cormen-et-al-s-algorithm-for-a-red-black-tree-delete-and-delete-fixup-functions-only.html

    Major attractiveness:
    1) No recursion
    2) No special tail-recursion optimization required
         (which is MUCH slower on some compilers)
    3) No need to do complicated transplants/
    4) No need to copy data (this is evil if we're templating our types)

  Change List:
    * 2024-12-12 Flip order of arguments to cnrbtree_generic_removebase, add RBREMOVE and RBGET.
    * 2025-02-07 Add stdint.h by default.
*/


#ifndef _CNRBTREE_H
#define _CNRBTREE_H

//XXX TODO: Consider optimizations and pulling even more things out of the templated code into the regular code.

#if !defined( CNRBTREE_MALLOC ) || !defined( CNRBTREE_FREE )
#include <stdlib.h>
#endif

//For creating trees and nodes - is best if data is initialized to zero.
#ifndef CNRBTREE_MALLOC
#define CNRBTREE_MALLOC(size) calloc( 1, size )
#endif

//For freeing trees and nodes.
#ifndef CNRBTREE_FREE
#define CNRBTREE_FREE free
#endif

#ifndef CNRBTREE_TEMPLATECODE
#define CNRBTREE_TEMPLATECODE 1
#endif

#ifndef CNRBTREE_GENERIC_DECORATOR
#define CNRBTREE_GENERIC_DECORATOR __attribute__((noinline))
#endif

#ifndef CNRBTREE_TEMPLATE_DECORATOR
#define CNRBTREE_TEMPLATE_DECORATOR static inline
#endif

#ifndef RBISNIL
#define RBISNIL( x )  ( RBTREEPUN(struct cnrbtree_generic_node_t*, typeof( x ), x) == &cnrbtree_nil )
#endif

//Shorthand for red-black access, and typesafe deletion.
#ifndef NO_RBA
#define RBA(x,y) (x->access)( x, y )->data
#define RBHAS(x,y) (!!(x->get)( x, y ))
#define RBGET(x,y) ((x->get)( x, y ))
#define RBDESTROY(x) (x->destroy)( x )
#define RBREMOVE(x,y) (cnrbtree_generic_removebase(RBTREEPUN(cnrbtree_generic*, typeof(x), x), RBTREEPUN(cnrbtree_generic_node*, typeof(y), y)))
#define RBFOREACH( type, tree, i ) for( cnrbtree_##type##_node * i = tree->begin; !RBISNIL( i ); i = RBTREEPUN( cnrbtree_##type##_node *, cnrbtree_generic_node *, cnrbtree_generic_next( RBTREEPUN( cnrbtree_generic_node *, cnrbtree_##type##_node *, i ) ) ) )
#endif

#if 0
#define RBTREEPUN(to, from, value) \
	 ((union { \
		 to x; \
		 from y; \
	 }){.y = value}).x
#else
#define RBTREEPUN(to, from, value)  ((to)(value))
#endif

#ifdef CNRBTREE_IMPLEMENTATION
	#include <stdint.h> // Needed for some of the functionality in this file.
#endif

struct cnrbtree_generic_node_t;
typedef struct cnrbtree_generic_node_t
{
	struct cnrbtree_generic_node_t * parent;
	struct cnrbtree_generic_node_t * left;
	struct cnrbtree_generic_node_t * right;
	char color;
} cnrbtree_generic_node;

struct cnrbtree_generic_t;
typedef struct cnrbtree_generic_t
{
	struct cnrbtree_generic_node_t * node;
	int size;
	cnrbtree_generic_node * (*access)( struct cnrbtree_generic_t * tree, void * key );
	cnrbtree_generic_node * (*get)( struct cnrbtree_generic_t * tree, void * key );
	void (*destroy)( struct cnrbtree_generic_t * tree );
	cnrbtree_generic_node * begin;
	cnrbtree_generic_node * tail;
} cnrbtree_generic;

//TODO: Consider collapsing this down to one bit.  Black == 0.
#define CNRBTREE_COLOR_NONE  0
#define CNRBTREE_COLOR_RED   1
#define CNRBTREE_COLOR_BLACK 2

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_removebase( cnrbtree_generic * t, cnrbtree_generic_node * n );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic_node * node );
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic_node * node );

extern struct cnrbtree_generic_node_t cnrbtree_nil;

#ifdef CNRBTREE_IMPLEMENTATION

struct cnrbtree_generic_node_t cnrbtree_nil = {
            .left = &cnrbtree_nil,
            .right = &cnrbtree_nil,
            .parent = &cnrbtree_nil,
            .color = CNRBTREE_COLOR_BLACK
};

//The syntax used inside these functions is a little odd. It is
// written to help hint to the compiler what can be optimized.
// after significant testing, it seems to provide an edge over
// writing node->parent over and over when it should just be
// stored to a temporary register.
CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_next( cnrbtree_generic_node * node )
{
	//Local reference, for compilers which don't know optimize global references well.
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * tmp;
	if( node == nil ) return 0;
	tmp = node->right;
	if( tmp != nil )
	{
		node = tmp;
		while( (tmp = node->left), tmp != nil ) node = tmp;
		return node;
	}
	tmp = node->parent;
	if( tmp != nil && node == tmp->left )
	{
		return tmp;
	}
	while( tmp != nil && tmp->right == node ) { node = tmp; tmp = node->parent; }
	return tmp;
}


CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_prev( cnrbtree_generic_node * node )
{
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * tmp;

	if( node == nil ) return 0;
	tmp = node->left;
	if( tmp != nil )
	{
		node = tmp;
		while( (tmp = node->right), tmp != nil ) node = tmp;
		return node;
	}
	tmp = node->parent;
	if( tmp != nil && node == tmp->right )
	{
		return tmp;
	}
	while( tmp != nil && tmp->left == node ) { node = tmp; tmp = node->parent; }
	return tmp;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateleft( cnrbtree_generic_node * n )
{
	/* From Wikipedia's RB Tree Page, seems slightly better than the CLRS model, but now that it's been
		modified, there seems to be very little difference between them. */
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * nnew = n->right;
	cnrbtree_generic_node * p = n->parent;
	cnrbtree_generic_node * nright = n->right = nnew->left;
	nnew->left = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( nright != nil ) {
		nright->parent = n;
	}
	/* Initially n could be the root. */
	if ( p != nil ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_rotateright( cnrbtree_generic_node * n )
{
	/* From Wikipedia's RB Tree Page */
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * nnew = n->left;
	cnrbtree_generic_node * p = n->parent;
	cnrbtree_generic_node * nleft = n->left = nnew->right;
	nnew->right = n;
	n->parent = nnew;
	/* Handle other child/parent pointers. */
	if ( nleft != nil ) {
		nleft->parent = n;
	}
	/* Initially n could be the root. */
	if ( p != nil ) {
		if (n == p->left) {
			p->left = nnew;
		} else if (n == p->right) {
			p->right = nnew;
		}
	}
	nnew->parent = p;
}

CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_insert_repair_tree_with_fixup( cnrbtree_generic_node * z, cnrbtree_generic * tree  )
{
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * zp;
	cnrbtree_generic_node * zpp;
	while( (( zp = z->parent ) != nil ) && zp->color == CNRBTREE_COLOR_RED )
	{
		zpp = zp->parent;

		cnrbtree_generic_node * u = (zp == zpp->left)?zpp->right:zpp->left;
		if( u->color == CNRBTREE_COLOR_RED )
		{
			//Case 1
			zp->color = CNRBTREE_COLOR_BLACK;
			u->color = CNRBTREE_COLOR_BLACK;
			zpp->color = CNRBTREE_COLOR_RED;
			z = zpp;
		}
		else
		{
			//Case 2 XXX Should we check cnrbtree_nil here?
			if( zp == zpp->left && z == zp->right )
			{
				cnrbtree_generic_rotateleft( zp );
				z = zp;
				zp = z->parent;
				zpp = zp->parent;
			}
			else if( zp == zpp->right && z == zp->left )
			{
				cnrbtree_generic_rotateright( zp );
				z = zp;
				zp = z->parent;
				zpp = zp->parent;
			}

			zp->color = CNRBTREE_COLOR_BLACK;

			//Case 3
			if( zpp != nil )
			{
				zpp->color = CNRBTREE_COLOR_RED;
				if( zpp->left == zp )
					cnrbtree_generic_rotateright( zpp );
				else
					cnrbtree_generic_rotateleft( zpp );
			}
		}
	}

	// Lastly, we must affix the root node's ptr correctly.
	while( (zp = z->parent), zp != nil ) { z = zp; }
	tree->node = z;
}

CNRBTREE_GENERIC_DECORATOR cnrbtree_generic_node * cnrbtree_generic_insert_repair_tree_with_fixup_primary( cnrbtree_generic_node * tmp, cnrbtree_generic * tree, int cmp, int sizetoalloc )
{
	cnrbtree_generic_node * ret;
	cnrbtree_generic_node * nil = &cnrbtree_nil;
	ret = CNRBTREE_MALLOC( sizetoalloc );

	ret->color = CNRBTREE_COLOR_RED;
	ret->left = nil;
	ret->right = nil;
	ret->parent = nil;

	/* Tricky shortcut for empty lists */
	tree->size++;

	if( tree->node == nil )
	{
		ret->parent = nil;
		ret->color = CNRBTREE_COLOR_BLACK; /* InsertCase1 from wikipedia */
		tree->node = ret;
		tree->begin = tree->tail = ret;
		return ret;
	}
	ret->parent = tmp;

	//XXX Should we protect 'tmp' to make sure it's not nil?
    if (cmp < 0) {
        tmp->left = ret;
        if(tmp == tree->begin)
		{
            tree->begin = ret;
		}
    }
    else {
        tmp->right = ret;
        if(tmp == tree->tail)
		{
            tree->tail = ret;
		}
    }

	/* Here, [ret] is the new node, it's red, and [tmp] is our parent */
	if( tmp->color == CNRBTREE_COLOR_RED )
	{
		cnrbtree_generic_insert_repair_tree_with_fixup( ret, tree );
	} /* Else InsertCase2 */

	return ret;
}

/////////////////DELETION//////////////////


CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_transplant( cnrbtree_generic * T, cnrbtree_generic_node * u, cnrbtree_generic_node * v )
{
	cnrbtree_generic_node * up = u->parent;
	if( up == &cnrbtree_nil )
		T->node = v;
	else if( u == up->left )
		up->left = v;
	else
		up->right = v;
	v->parent = u->parent; //Not sure what algorithm witchcraft is going on here, but everything breaks if you "protect" this from nil.
}

//"RB-DELETE(T, z)"
CNRBTREE_GENERIC_DECORATOR void cnrbtree_generic_removebase( cnrbtree_generic * T, cnrbtree_generic_node * z )
{
	T->size--;

	if(z == T->begin)
		T->begin = cnrbtree_generic_next(z);
	if(z == T->tail)
		T->tail = cnrbtree_generic_prev(z);

	cnrbtree_generic_node * nil = &cnrbtree_nil;
	cnrbtree_generic_node * x;
	cnrbtree_generic_node * y = z;
	char y_original_color = y->color;

	if( z->left == nil )
	{
		x = z->right;
		cnrbtree_generic_transplant( T, z, x );
	}
	else if( z->right == nil )
	{
		x = z->left;
		cnrbtree_generic_transplant( T, z, x );		
	}
	else
	{
		// XXX How is it possible that this never fails?! I would have expected to need to check if nil and if so, do cnrbtree_generic_prev.
		y = cnrbtree_generic_next( z ); 

		y_original_color = y->color;
		cnrbtree_generic_node * tmp = y->right;

		x = tmp; //I would be concerned if X is nil, but that appears to be OK.

		if( y->parent == z )
		{
			x->parent = y;
		}
		else
		{
			cnrbtree_generic_transplant( T, y, tmp );
			tmp = y->right = z->right;
			tmp->parent = y;
		}

		cnrbtree_generic_transplant( T, z, y );

		tmp = y->left = z->left;
		tmp->parent = y;
		y->color = z->color;
	}
	if( y_original_color == CNRBTREE_COLOR_BLACK )
	{
		//"RB-DELETE-FIXUP( T,x )"
		cnrbtree_generic_node * w;
		cnrbtree_generic_node * xp;

		while( x->color == CNRBTREE_COLOR_BLACK )
		{
			xp = x->parent;
			if( x == xp->left )
			{
				w = xp->right;
				if( w->color == CNRBTREE_COLOR_RED )
				{
					w->color = CNRBTREE_COLOR_BLACK;
					xp->color = CNRBTREE_COLOR_RED;
					cnrbtree_generic_rotateleft( xp );
					w = xp->right;
				}
				if( ( w->left->color == CNRBTREE_COLOR_BLACK ) && 
					( w->right->color == CNRBTREE_COLOR_BLACK ) )
				{
					w->color = CNRBTREE_COLOR_RED;
					x = xp;
				}
				else
				{
					if( w->right->color == CNRBTREE_COLOR_BLACK )
					{
						w->left->color = CNRBTREE_COLOR_BLACK;
						w->color = CNRBTREE_COLOR_RED;
						cnrbtree_generic_rotateright( w );
						w = xp->right;
					}
					w->color = x->parent->color;
					xp->color = CNRBTREE_COLOR_BLACK;
					w->right->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateleft( xp );
					break;
				}
			}
			else
			{
				//Same as above but inverted sides.
				w = xp->left;
				if( w->color == CNRBTREE_COLOR_RED )
				{
					w->color = CNRBTREE_COLOR_BLACK;
					xp->color = CNRBTREE_COLOR_RED;
					cnrbtree_generic_rotateright( xp );
					w = xp->left;
				}
				if( ( w->right->color == CNRBTREE_COLOR_BLACK ) && 
					( w->left->color == CNRBTREE_COLOR_BLACK ) )
				{
					w->color = CNRBTREE_COLOR_RED;
					x = xp;
				}
				else
				{
					if( w->left->color == CNRBTREE_COLOR_BLACK )
					{
						w->right->color = CNRBTREE_COLOR_BLACK;
						w->color = CNRBTREE_COLOR_RED;
						cnrbtree_generic_rotateleft( w );
						w = xp->left;
					}
					w->color = xp->color;
					x->parent->color = CNRBTREE_COLOR_BLACK;
					w->left->color = CNRBTREE_COLOR_BLACK;
					cnrbtree_generic_rotateright( xp );
					break;
				}
			}
		}

		x->color = CNRBTREE_COLOR_BLACK;

		// We must affix the root node's ptr correctly.
		while( (xp = x->parent), xp != nil ) { x = xp; }
		T->node = x;

		//End "RB-DELETE-FIXUP( T,x )"
	}

	if( T->size == 0 ) { 
		T->node = nil;
	}

	CNRBTREE_FREE( z );

	return;
}

#endif // CNRBTREE_IMPLEMENTATION


//This is the template generator.  This is how new types are created.
#define CNRBTREETYPETEMPLATE( key_t, data_t ) \
	struct cnrbtree_##key_t##data_t##_node_t; \
	typedef union \
	{ \
		cnrbtree_generic_node g; \
		struct { \
			cnrbtree_generic_node * parent; \
			cnrbtree_generic_node * left; \
			cnrbtree_generic_node * right; \
			char color; \
			key_t key; \
			data_t data; \
		};\
	} cnrbtree_##key_t##data_t##_node; \
	typedef union cnrbtree_##key_t##data_t##_\
	{ \
		cnrbtree_generic g; \
		struct { \
			cnrbtree_##key_t##data_t##_node * node; \
			int size; \
			cnrbtree_##key_t##data_t##_node * (*access)( union cnrbtree_##key_t##data_t##_ * tree, key_t key ); \
			cnrbtree_##key_t##data_t##_node * (*get)( union cnrbtree_##key_t##data_t##_ * tree, key_t key ); \
			void (*destroy)( union cnrbtree_##key_t##data_t##_ * tree ); \
			cnrbtree_##key_t##data_t##_node * begin; \
			cnrbtree_##key_t##data_t##_node * tail; \
		}; \
	} cnrbtree_##key_t##data_t; \

#if CNRBTREE_TEMPLATECODE

#define CNRBTREETEMPLATE( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ) \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = (cnrbtree_##key_t##data_t##_node*)&cnrbtree_nil; \
		cnrbtree_##key_t##data_t##_node * tmp = tree->node; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		while( tmp != nil ) \
		{ \
			int cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node*)tmp->left; \
			else if( cmp > 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node*)tmp->right; \
			else return tmp; \
			if( tmpnext == nil ) \
			{ \
				return approx?tmp:0; \
			} \
			tmp = tmpnext; \
		} \
		return 0; \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		return cnrbtree_##key_t##data_t##_get2( tree, key, 0 ); \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_access( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = (cnrbtree_##key_t##data_t##_node *)&cnrbtree_nil; \
		/* This function could utilize cnrbtree_##key_t##data_t##_get2 but would require an extra compare */ \
		cnrbtree_##key_t##data_t##_node * tmp = tree->node;  \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		int cmp = 0; \
		while( tmp != nil ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node*)tmp->left; \
			else if( cmp > 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node*)tmp->right; \
			else return tmp; \
			if( tmpnext == nil ) break; \
			tmp = tmpnext; \
		} \
		cnrbtree_##key_t##data_t##_node * ret; \
		ret = RBTREEPUN(cnrbtree_##key_t##data_t##_node *, cnrbtree_generic_node*, cnrbtree_generic_insert_repair_tree_with_fixup_primary( \
			RBTREEPUN(cnrbtree_generic_node*, cnrbtree_##key_t##data_t##_node *, tmp ), RBTREEPUN(cnrbtree_generic*, cnrbtree_##key_t##data_t*, tree ), \
			cmp, (int)sizeof( cnrbtree_##key_t##data_t##_node ) ) ); \
		copykeyxy( ret->key, key, ret->data ); \
		return ret; \
	}\
	\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_remove( cnrbtree_##key_t##data_t * tree, key_t key ) \
	{\
		cnrbtree_##key_t##data_t##_node * nil = (cnrbtree_##key_t##data_t##_node *)&cnrbtree_nil; \
		cnrbtree_##key_t##data_t##_node * tmp = 0; \
		cnrbtree_##key_t##data_t##_node * tmpnext = 0; \
		if( tree->node == nil ) return; \
		tmp = tree->node; \
		int cmp; \
		while( 1 ) \
		{ \
			cmp = comparexy( key, tmp->key ); \
			if( cmp < 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node *)tmp->left; \
			else if( cmp > 0 ) tmpnext = (cnrbtree_##key_t##data_t##_node *)tmp->right; \
			else break; \
			if( tmpnext == nil ) return; \
			tmp = tmpnext; \
		} \
		/* found an item, tmp, to delete. */ \
		deletekeyxy( tmp->key, tmp->data ); \
		cnrbtree_generic_removebase( RBTREEPUN(cnrbtree_generic*, cnrbtree_##key_t##data_t *, tree), RBTREEPUN( cnrbtree_generic_node *, cnrbtree_##key_t##data_t##_node *, tmp ) ); \
	} \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t * tree, cnrbtree_##key_t##data_t##_node * node ) \
	{\
		cnrbtree_generic_node * nil = &cnrbtree_nil; \
		if( (cnrbtree_generic_node *)node == nil ) return; \
		deletekeyxy( node->key, node->data ); \
		if( node->left != nil ) cnrbtree_##key_t##data_t##_destroy_node_internal( tree, (cnrbtree_##key_t##data_t##_node *) node->left ); \
		if( node->right != nil ) cnrbtree_##key_t##data_t##_destroy_node_internal( tree, (cnrbtree_##key_t##data_t##_node *)node->right ); \
		CNRBTREE_FREE( node ); \
	}\
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ) \
	{\
		cnrbtree_##key_t##data_t##_destroy_node_internal( tree, tree->node ); \
		CNRBTREE_FREE( tree ); \
	} \
	\
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create(void) \
	{\
		cnrbtree_##key_t##data_t * ret = CNRBTREE_MALLOC( sizeof( cnrbtree_##key_t##data_t ) ); \
		cnrbtree_##key_t##data_t##_node * nil = RBTREEPUN( cnrbtree_##key_t##data_t##_node *, cnrbtree_generic_node *, &cnrbtree_nil ); \
		ret->node = nil; \
		ret->tail = nil; \
		ret->begin = nil; \
		ret->size = 0; \
		ret->access = cnrbtree_##key_t##data_t##_access; \
		ret->get = cnrbtree_##key_t##data_t##_get; \
		ret->destroy = cnrbtree_##key_t##data_t##_destroy; \
		return ret; \
	}\

#else
#define CNRBTREETEMPLATE CNRBTREETEMPLATE_DEFINITION
#endif


#define CNRBTREETEMPLATE_DEFINITION( key_t, data_t, comparexy, copykeyxy, deletekeyxy ) \
	CNRBTREETYPETEMPLATE( key_t, data_t ) \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get2( cnrbtree_##key_t##data_t * tree, key_t key, int approx ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_get( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t##_node * cnrbtree_##key_t##data_t##_access( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_remove( cnrbtree_##key_t##data_t * tree, key_t key ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy_node_internal( cnrbtree_##key_t##data_t * tree, cnrbtree_##key_t##data_t##_node * node ); \
	CNRBTREE_TEMPLATE_DECORATOR void cnrbtree_##key_t##data_t##_destroy( cnrbtree_##key_t##data_t * tree ); \
	CNRBTREE_TEMPLATE_DECORATOR cnrbtree_##key_t##data_t * cnrbtree_##key_t##data_t##_create(); \

#define RBstrcmp(x,y) strcmp(x,y)
#define RBstrcpy(x,y,z) { x = strdup(y); }
#define RBstrstrcpy(x,y,z) { x = strdup(y); z = 0; }
#define RBstrdel(x,y) free( x );
#define RBstrstrdel( x,z ) { free(x); free(z); }
#define RBCBSTR RBstrcmp, RBstrcpy, RBstrdel


#define RBptrcmpnomatch(x,y) (((((intptr_t)x-(intptr_t)y)<0)?-1:1))
#define RBptrcmp(x,y) ((x==y)?0:((((intptr_t)x-(intptr_t)y)<0)?-1:1))
#define RBptrcpy(x,y,z) { x = y; }
#define RBnullop(x,y)
#define RBCBPTR RBptrcmp, RBptrcpy

#ifndef CNRBTREE_NO_SETTYPES

#include <string.h>

//Code for pointer-sets (cnptrset) - this is only for void *
typedef void * rbset_t;
typedef char rbset_null_t[];
#ifdef CNRBTREE_IMPLEMENTATION
	CNRBTREETEMPLATE( rbset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBnullop )
#else
	CNRBTREETEMPLATE_DEFINITION( rbset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBnullop )
#endif

typedef cnrbtree_rbset_trbset_null_t cnptrset;
#define cnptrset_create() cnrbtree_rbset_trbset_null_t_create()
#define cnptrset_insert( st, key ) cnrbtree_rbset_trbset_null_t_access( st, key )
#define cnptrset_remove( st, key ) cnrbtree_rbset_trbset_null_t_remove( st, key )
#define cnptrset_destroy( st ) cnrbtree_rbset_trbset_null_t_destroy( st )
//Note, you need a pre-defined void * for the type in the iteration. i.e. void * i; cnptrfset_foreach( tree, i );
#define cnptrset_foreach( tree, i ) \
	for( cnrbtree_rbset_trbset_null_t_node * node##i = tree->begin; \
		(node##i != &cnrbtree_nil) && (i = (node##i)->key) != 0 ) \
		node##i = RBTREEPUN(cnrbtree_rbset_trbset_null_t_node *, cnrbtree_generic_node *, cnrbtree_generic_next( RBTREEPUN( cnrbtree_generic_node *, cnrbtree_rbset_trbset_null_t_node, node##i ) );



//Code for string-sets (cnstrset) - this is only for void *
typedef char * rbstrset_t;
#ifdef CNRBTREE_IMPLEMENTATION
	CNRBTREETEMPLATE( rbstrset_t, rbset_null_t, RBstrcmp, RBstrcpy, RBstrdel )
	CNRBTREETEMPLATE( rbstrset_t, rbstrset_t, RBstrcmp, RBstrstrcpy,  )
#else
	CNRBTREETEMPLATE_DEFINITION( rbstrset_t, rbset_null_t, RBptrcmp, RBptrcpy, RBstrdel )
	CNRBTREETEMPLATE_DEFINITION( rbstrset_t, rbstrset_t, RBstrcmp, RBstrstrcpy,  )
#endif


typedef cnrbtree_rbstrset_trbset_null_t cnstrset;
#define cnstrset_create() cnrbtree_rbstrset_trbset_null_t_create()
#define cnstrset_insert( st, key ) cnrbtree_rbstrset_trbset_null_t_access( st, key )
#define cnstrset_remove( st, key ) cnrbtree_rbstrset_trbset_null_t_remove( st, key )
#define cnstrset_destroy( st ) cnrbtree_rbstrset_trbset_null_t_destroy( st )
//Note, you need a pre-defined char * for the type in the iteration. i.e. char * i; cnstrfset_foreach( tree, i );
#define cnstrset_foreach( tree, i ) \
	for( cnrbtree_rbstrset_trbset_null_t_node * node##i = tree->begin; \
		i = (node##i)->key, &(node##i)->g != &cnrbtree_nil; \
		node##i = (cnrbtree_rbstrset_trbset_null_t_node *)cnrbtree_generic_next( (cnrbtree_generic_node *)node##i ) )


typedef cnrbtree_rbstrset_trbstrset_t cnstrstrmap;
#define cnstrstrmap_create() cnrbtree_rbstrset_trbstrset_t_create()
#define cnstrstrmap_insert( st, key ) cnrbtree_rbstrset_trbstrset_t_access( st, key )
#define cnstrstrmap_remove( st, key ) cnrbtree_rbstrset_trbstrset_t_remove( st, key )
#define cnstrstrmap_destroy( st ) cnrbtree_rbstrset_trbstrset_t_destroy( st )
//Note, you need a pre-defined char * for the type in the iteration. i.e. char * i; cnstrfset_foreach( tree, i );
#define cnstrstrmap_foreach( tree, i, d ) \
	for( cnrbtree_rbstrset_trbstrset_t_node * node##i = tree->begin; \
		i = (node##i)->key, d = (node##i)->data, &(node##i)->g != &cnrbtree_nil; \
		node##i = (cnrbtree_rbstrset_trbstrset_t_node *)cnrbtree_generic_next( (cnrbtree_generic_node *)node##i ) )

#endif

#endif




