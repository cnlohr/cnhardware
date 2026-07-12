#ifndef _KICADPARSE_H
#define _KICADPARSE_H

#define CNRBTREE_IMPLEMENTATION
#include "cnrbtree.h"

union cnrbtree_strkicadElementPtr_;
typedef union cnrbtree_bNamekicadElementPtr_t cnrbtree_bNamekicadElementPtr_;

#define KIBR( e, name ) (b ? ( RBHAS(e->b, name ) ? RBA( e->b, name ) : 0 ) : 0 )

struct kicadElement_t
{
	int branchCount;
	int leafCount;
	char * name;
	struct kicadElement_t ** branches;
	union cnrbtree_bNamekicadElementPtr_ * b;
	char ** leaves;
};

typedef struct kicadElement_t kicadElement;
typedef struct kicadElement_t * kicadElementPtr;
typedef char * bName;
CNRBTREETEMPLATE( bName, kicadElementPtr, RBstrcmp, RBstrcpy, RBstrdel );


kicadElement * ParseKicadFile( char * str, int len, int * c, int * lineno, int depth )
{
	depth++;

	for( ; *c < len; (*c)++ )
	{
		if( str[*c] == '\r' ) { str[*c] = 0; continue; }
		if( str[*c] == '\n' ) (*lineno)++;
		else if( str[*c] == '\t' || str[*c] == ' ' ) continue;
		else if( str[*c] == '(' ) break;
		else { c++; return 0; }
	}
	(*c)++;
	int empty = 0;
	char * nameStart = str + *c;
	for( ; *c < len; (*c)++ )
	{
		if( str[*c] == '\r' ) { str[*c] = 0; continue; }
		if( str[*c] == '\n' ) { (*lineno)++; (*c)++; break; }
		else if( str[*c] == ')' ) { (*c)++; empty = 1; break; }
		else if( str[*c] == ' ' || str[*c] == '\t' ) { (*c)++; break; }
	}

	if( str[*c] == '\r' ) { str[*c] = 0; (*c)++; }
	if( str[*c] == '\n' ) (*lineno)++;

	// leaf or more branches.
	char * nameend = str + (*c) - 1;
	kicadElement * ret = calloc( sizeof( kicadElement ), 1 );
	ret->name = nameStart;
	char * elem = str + *c;
	int quotes = 0;
	int startedleaf = 0;

	if( !empty )
	for( ; *c < len; (*c)++ )
	{
		if( str[*c] == '\r' ) { str[*c] = 0; continue; }
		if( str[*c] == '\n' ) (*lineno)++;
		if( !startedleaf && ( str[*c] == ' ' || str[*c] == '\t' ) ) continue;
		if( !startedleaf && str[*c] == '(' )
		{
			// A branch.
			kicadElement * e = ParseKicadFile( str, len, c, lineno, depth );
			if( !e )
			{
				fprintf( stderr, "Parse fail on line %d\n", *lineno );
				break;
			}
			int b = ret->branchCount;
			ret->branchCount++;
			ret->branches = realloc( ret->branches, sizeof( kicadElement * ) * ret->branchCount );
			ret->branches[b] = e;
			startedleaf = 0;
			if( str[*c] == '\r' ) continue;
			if( str[*c] == '\n' ) (*lineno)++;
			elem = str + *c + 1;
		}
		else if( str[*c] == '\"' )
		{
			quotes = !quotes;
			startedleaf = 1;
		}
		else if( !quotes )
		{
			int term = str[*c] == ')';
			startedleaf = 1;
			if( str[*c] == ' ' || str[*c] == '\t' || str[*c] == '\n' || str[*c] == '\r' || term )
			{

				str[*c] = 0;

				int b = ret->leafCount;
				ret->leafCount++;
				ret->leaves = realloc( ret->leaves, sizeof( char * ) * ret->leafCount );
				ret->leaves[b] = elem;
				startedleaf = 0;

				elem = str + *c + 1;
			}

			if( term )
			{
				str[*c] = 0;
				break;
			}
		}
		else
		{
			startedleaf = 1;
		}
	}
	*nameend = 0;
	(*c)++;

	ret->b = cnrbtree_bNamekicadElementPtr_create();
	for( int i = 0; i < ret->branchCount; i++ )
	{
		kicadElement * a = ret->branches[i];
		RBA( ret->b, a->name ) = a;
	}

	return ret;
}


#endif

