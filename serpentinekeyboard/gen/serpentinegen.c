// TODO:
// 1. Make line segment measurements use conic sections instead of lines.
// 2. Hint where the red dot is.


// Approach 1:
//   0.17mm Output, 118000 segments, targetRadius = 4   about 18 meters.  Start by upping spring force to abotu 1.7 and dt to about 0.3
//   0.13mm Output, 198000 segments. targetRadius = 2.8 -- this does not bring joy. 0.26mm line-to-line clearance @ 25.7 meters.  -> Actual idea:
//      switch to 0.135mm output, 0.243mm line-to-line,  30M length
//			Force:2.85/Spring:1.61/Size:0.75(2.1 cr)/Dt=0.011
//   Then manual fixup.

// Aproach 2:
//  158000 segments, 0.15 thick, target 23 meters.

#pragma comment(lib, "opengl32")
#pragma comment(lib, "user32")
#pragma comment(lib, "gdi32")

#include <stdatomic.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <immintrin.h>

#define CNFG_IMPLEMENTATION
#define CNFGOGL

#include "os_generic.h"
#include "rawdraw_sf.h"

#include "kicadparse.h"

int threadct = 24;

kicadElement * kicad_file;

#define MIN( x, y ) ((x)<(y)?(x):(y))

#define ELEMENTS 158000
#define OUTTHICK 0.15

#define W 1050
#define H 1000
#define MSPHX 320

// Must be a multiple of 32 or else the SIMD code will burn your house down
#define WORRY_ABOUT_LEN 1024

#define BOTTOM

#ifdef BOTTOM
#define USENET "\"RBOT\""
#define USECOPPER "\"B.Cu\""
#define ADHESIVEUSE "\"B.Adhes\""
#define OUTFILE "../testboard2/test.pretty/testboard_back.kicad_mod"
#else
#define USENET "\"RTOP\""
#define USECOPPER "\"F.Cu\""
#define ADHESIVEUSE "\"F.Adhes\""
#define OUTFILE "../testboard2/test.pretty/testboard.kicad_mod"
#endif

#define DFLT double

int enable_line_collision = 0;
DFLT inter_force_multiplier = 1.0;
DFLT targetRadius = 2.8;
DFLT radSpeed = 0.0000003; // Speed by which the lengths approach the target length.
int  maxRadSpeedSpeedup = 32000;
DFLT velocityDamp = 1;
DFLT sphForceCoeffStart = .1;
DFLT sphForceCoeffFinal = 10;
DFLT sphForceMax = 1e10;
DFLT edgeForce = .9;
DFLT dt = 0.03;
DFLT springForceStart = 3;
DFLT springForceFinal = 3;
DFLT springForceUserMux = 1.0;
DFLT sizeUserMux = 1.0;
DFLT tetherShrink = 0.0; // Set to 0 to make it something that desires to be as tight as possible.
DFLT speedUserMux = 1.0;
DFLT closestPts = 1e20;
int closestAt = -1;

DFLT xofs;
DFLT yofs;
DFLT zoom = 1;
int dragdown;
DFLT dragatx;
DFLT dragaty;
int grabdown;
int grabbede = -1;
short cww, cwh;
int paused = 0;
int renderBubbles = 0;

DFLT loadOffsetX;
DFLT loadOffsetY;


struct serpElem
{
	DFLT x, y;
	DFLT fx, fy;
	DFLT vx, vy;
	DFLT currentRadius;
	DFLT finalRadius;
	DFLT radSpeed;
	DFLT applyForce;
	char fixed, pinned;
};

struct serpElem elems[ELEMENTS];

#define EMX ((W))
#define EMY ((H))
int elementMap[EMX][EMY][MSPHX];
_Atomic int elementMapCnt[EMX][EMY];
int frameno;

DFLT barrierMap[EMX][EMY];
uint32_t barrierMapTextureData[EMX * EMY];

DFLT sphForceCoeff; // computed each frame

int * starts;
int * ends;
og_thread_t * threads;
og_sema_t * threadstarts;
og_sema_t * threaddones;
int doquit = 0;

int thits = 0;
int ttotal = 0;

void HandleKey( int keycode, int bDown )
{
	if( !bDown ) return;
	switch( keycode )
	{
	case 'R': case 'r':
		inter_force_multiplier *= 0.909090909090909;
		break;
	case 'F': case 'f':
		inter_force_multiplier *= 1.1;
		break;
	case 'T': case 't':
		springForceUserMux *= 0.909090909090909;
		break;
	case 'G': case 'g':
		springForceUserMux *= 1.1;
		if( springForceUserMux > 5 ) springForceUserMux = 5;
		break;
	case 'Y': case 'y':
		sizeUserMux *= 0.909090909090909;
		break;
	case 'H': case 'h':
		sizeUserMux *= 1.1;
		break;
	case 'U': case 'u':
		speedUserMux *= 0.909090909090909;
		break;
	case 'J': case 'j':
		speedUserMux *= 1.1;
		break;
	case 'I': case 'i':
		dt *= 0.909090909090909;
		break;
	case 'K': case 'k':
		dt *= 1.1;
		if( dt > 0.06 ) dt = 0.06;
		break;
	case ' ':
		paused = !paused;
		break;
	case 'b': case 'B':
		renderBubbles = !renderBubbles;
		break;
	case 'L': case 'l': enable_line_collision = !enable_line_collision; break;
	case 'z': case 'Z':
		if( closestAt >= 0 )
		{
			DFLT centerx = cww/2.0;
			DFLT centery = cwh/2.0;
			xofs = centerx-elems[closestAt].x*zoom;
			yofs = centery-elems[closestAt].y*zoom;
		}		
		break;
	case 's':
	case 'S':
	{
		int padno = 1;
		FILE * f = fopen( OUTFILE, "w" );
		fprintf( f, "(footprint \"testboard\"\n\
	(version 20241229)\n\
	(generator \"pcbnew\")\n\
	(generator_version \"9.0\")\n\
	(layer \n\"F.Cu\")\n\
	(property \"Reference\" \"REF**\"\n\
		(at 0 -0.5 0)\n\
		(unlocked yes)\n\
		(layer \"F.SilkS\")\n\
		(uuid \"116ebc4b-0d94-4cf3-bcbd-dd4012b75f71\")\n\
		(effects\n\
			(font\n\
				(size 1 1)\n\
				(thickness 0.1)\n\
			)\n\
		)\n\
	)\n");

		int e = 0;
		for( e = 0; e < ELEMENTS; e++ )
		{
			int n = e+1;
			struct serpElem * et = elems + e;
			struct serpElem * en = elems + n;

			if( e < ELEMENTS-1 )
			{
				fprintf( f, "\n\
	(fp_line\n\
		(start %f %f)\n\
		(end %f %f)\n\
		(stroke\n\
			(width %.4f)\n\
			(type default)\n\
		)\n\
		(layer %s)\n\
		(uuid \"76f6467a-8595-40b1-8617-6eeb%08x\")\n\
	)\n", et->x/10.0, et->y/10.0, en->x/10.0, en->y/10.0, OUTTHICK, USECOPPER, e );
			}
		}

		fprintf( f, "\n\
	(embedded_fonts no)\n\
)\n" );
		fclose( f );


	}
		break;
	}
}

void HandleButton( int x, int y, int button, int bDown )
{
	if( bDown )
	{
		switch( button )
		{
		case 1:
		{
			grabdown = 1;
			dragatx = x;
			dragaty = y;

			float invx = (x - xofs)/zoom;
			float invy = (y - yofs)/zoom;

			int e, sele = -1;
			float mindist = 1e20;
			for( e = 0; e < ELEMENTS; e++ )
			{
				struct serpElem * el = &elems[e];
				float dx = el->x - invx;
				float dy = el->y - invy;
				float dist = sqrt( dx*dx+dy*dy );
				if( dist > el->currentRadius ) continue;
				if( el->fixed ) continue;
				if( dist > mindist ) continue;
				mindist = dist;
				sele = e;
			}
			grabbede = sele;

			break;
		}
		case 2:
		case 3:
			dragdown = 1;
			dragatx = x;
			dragaty = y;
			break;
		case 14:
		case 4:
			xofs -= x;
			yofs -= y;
			xofs /= zoom;
			yofs /= zoom;
			zoom *= 1.25;
			xofs *= zoom;
			yofs *= zoom;
			xofs += x;
			yofs += y;
			break;
		case 15:
		case 5:
			xofs -= x;
			yofs -= y;
			xofs /= zoom;
			yofs /= zoom;
			zoom /= 1.25;
			xofs *= zoom;
			yofs *= zoom;
			xofs += x;
			yofs += y;
			break;
		default:
			printf( "Button: %d\n", button );
			break;
		}
	}

	if( !bDown )
	{
		switch( button )
		{
		case 1:
			if( grabdown && grabbede >= 0 )
			{
				elems[grabbede].pinned = !elems[grabbede].pinned;
				elems[grabbede].fx = 0;
				elems[grabbede].fy = 0;
				elems[grabbede].vx = 0;
				elems[grabbede].vy = 0;
			}
			grabbede = -1;
			grabdown = 0;
			break;
		case 2:
		case 3:
			dragdown = 0;
			break;
		}
	}
}

void HandleMotion( int x, int y, int mask )
{
	if( dragdown )
	{
		DFLT dx = x - dragatx;
		DFLT dy = y - dragaty;
		xofs += dx;
		yofs += dy;
		dragatx = x;
		dragaty = y;
	}
	if( grabdown )
	{
		DFLT dx = x - dragatx;
		DFLT dy = y - dragaty;

		if( grabbede >= 0 )
		{
			struct serpElem * el = &elems[grabbede];
			el->x += dx / zoom;
			el->y += dy / zoom;
		}
		dragatx = x;
		dragaty = y;
	}
}

int HandleDestroy() { return 0; }

void execthread_p1(int start, int end)
{
		int e;
		for( e = start; e < end; e++ )
		{
			DFLT tx = elems[e].x;
			DFLT ty = elems[e].y;
			DFLT crad = elems[e].currentRadius;

			int seCxMin = (tx-1 - crad);
			int seCyMin = (ty-1 - crad);
			int seCxMax = (tx+1 + crad);
			int seCyMax = (ty+1 + crad);

			if( seCxMin < 0 ) seCxMin = 0;
			if( seCyMin < 0 ) seCyMin = 0;
			if( seCxMin >= EMX ) seCxMin = EMY;
			if( seCyMin >= EMY ) seCyMin = EMY;

			if( seCxMax < 0 ) seCxMax = 0;
			if( seCyMax < 0 ) seCyMax = 0;
			if( seCxMax >= EMX ) seCxMax = EMY;
			if( seCyMax >= EMY ) seCyMax = EMY;

			for( int cx = seCxMin; cx <= seCxMax; cx++ )
			for( int cy = seCyMin; cy <= seCyMax; cy++ )
			{
				float midx = cx - tx;
				float midy = cy - ty;
				if( midx*midx+midy*midy > crad*crad+1 ) continue;

				if( cx < 0 || cy < 0 || cx >= EMX || cy >= EMY ) continue;

				//int ct = ++elementMapCnt[cx][cy];
				int ct = atomic_fetch_add( &elementMapCnt[cx][cy], 1 );
				if( ct >= MSPHX )
					fprintf( stderr, "CT Invalid %d\n", ct );
				else
					elementMap[cx][cy][ct] = e;
			}
		}
}

void execthread_p2(int start, int end, int* worryAbout)
{
		int e;
		int tchecks = 0;
		for( e = start; e < end; e++ )
		{
			int worryAboutCnt = 0;

			DFLT tx = elems[e].x;
			DFLT ty = elems[e].y;

			DFLT crad = elems[e].currentRadius * .5;

			int seCxMin = (tx-1 - crad);
			int seCyMin = (ty-1 - crad);
			int seCxMax = (tx+1 + crad);
			int seCyMax = (ty+1 + crad);

			if( seCxMin < 0 ) seCxMin = 0;
			if( seCyMin < 0 ) seCyMin = 0;
			if( seCxMin >= EMX ) seCxMin = EMY;
			if( seCyMin >= EMY ) seCyMin = EMY;

			if( seCxMax < 0 ) seCxMax = 0;
			if( seCyMax < 0 ) seCyMax = 0;
			if( seCxMax >= EMX ) seCxMax = EMY;
			if( seCyMax >= EMY ) seCyMax = EMY;

			for( int cx = seCxMin; cx <= seCxMax; cx++ )
			for( int cy = seCyMin; cy <= seCyMax; cy++ )
			{
				if( cx < 0 || cy < 0 || cx >= EMX || cy >= EMY ) continue;
				int * se = elementMap[cx][cy];
				int ct = elementMapCnt[cx][cy];

				for (int c = 0; c < ct; c++)
				{
					int ec = se[c];
					if (ec == e) { continue; }

#ifdef USE_AVX512
					__m512i ECVec = _mm512_set1_epi32(ec);
#else
					__m256i ECVec = _mm256_set1_epi32(ec);
#endif
					int VecN = 0;
					for (VecN = 0; VecN < worryAboutCnt; VecN += 32)
					{
#ifdef USE_AVX512
						uint16_t Compare0 = _mm512_cmpeq_epi32_mask(_mm512_loadu_si512((__m512i*)&worryAbout[VecN]), ECVec);
						uint16_t Compare1 = _mm512_cmpeq_epi32_mask(_mm512_loadu_si512((__m512i*)&worryAbout[VecN + 16]), ECVec);
						uint32_t Compare = (Compare1 << 16) | Compare0;
#else
						uint8_t Compare0 = _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_loadu_si256((__m256i*)&worryAbout[VecN]), ECVec)));
						uint8_t Compare1 = _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_loadu_si256((__m256i*)&worryAbout[VecN + 8]), ECVec)));
						uint8_t Compare2 = _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_loadu_si256((__m256i*)&worryAbout[VecN + 16]), ECVec)));
						uint8_t Compare3 = _mm256_movemask_ps(_mm256_castsi256_ps(_mm256_cmpeq_epi32(_mm256_loadu_si256((__m256i*)&worryAbout[VecN + 24]), ECVec)));
						// The following is the most expensive line of code in the entire program now.
						// Compare0..3 must all be registers (the SIMD instruction can only output to reg), so can't do any funny stack tricks to re-interpret them as an int in-place.
						uint32_t Compare = (Compare3 << 24) | (Compare2 << 16) | (Compare1 << 8) | Compare0;
#endif
						int ItemsToCheck = MIN(32, worryAboutCnt - VecN);
						uint32_t CompareMasked = Compare << (32 - ItemsToCheck);

						if (CompareMasked != 0) { break; } // This still has pretty large misprediction rate, but vectorizing out the compares has made it much less of an issue
					}

					if (VecN >= worryAboutCnt)
					{
						worryAboutCnt++;
						if (worryAboutCnt >= WORRY_ABOUT_LEN)
							fprintf(stderr, "Warning: worryAbout too small\n");
						else
							worryAbout[worryAboutCnt - 1] = ec;
					}
				}
			}


			for( int cx = seCxMin-1; cx <= seCxMax+1; cx++ )
			for( int cy = seCyMin-1; cy <= seCyMax+1; cy++ )
			{
				// Check barriers.
				if( cx >= EMX || cy >= EMX || cx < 0 || cy < 0 ) continue;
				DFLT b = barrierMap[cx][cy];
				if( b > 0.01 )
				{
					//XXX REWRITE THIS.
					float bcx = cx - tx;
					float bcy = cy - ty;
					float dist = sqrt( bcx * bcx + bcy * bcy );
					b = 1.7;
					b -= dist;
					b += crad;
					//printf( "%f-%f %f-%f  %f %f\n", (float)cx, tx, (float)cy,ty,dist,crad );
					b *= edgeForce;
					if( b > 0.0 )
					{
						float px = -bcx * b;
						float py = -bcy * b;
						elems[e].fx += px;
						elems[e].fy += py;
					}
				}
			}

			ttotal += worryAboutCnt;
			for( int c = 0; c < worryAboutCnt; c++ )
			{
				int cc = worryAbout[c];
				DFLT cx = elems[cc].x;
				DFLT cy = elems[cc].y;
				DFLT cor = elems[cc].currentRadius;
				// Previously only did point-line collision.
				DFLT dx = cx - tx;
				DFLT dy = cy - ty;
				DFLT dist = sqrtf( dx * dx + dy * dy );
				DFLT dforce = (crad+cor) - dist;

				// dforce is now whether or not we are hitting the sphere intersect, but we need
				// XXX TODO Handle segment intersection.
				int skipnext = 0;
				
				int comp = (cc < ELEMENTS-1)?(cc+1):(cc-1);
				if( comp != e && enable_line_collision )
				{
					DFLT segCX = elems[comp].x;
					DFLT segCY = elems[comp].y;
					DFLT segCOR = elems[comp].currentRadius;
					DFLT PAx = tx - cx;
					DFLT PAy = ty - cy;
					DFLT BAx = segCX - cx;
					DFLT BAy = segCY - cy;
					// Inigo Quilez, Segment-Exact
					float t = (PAx*BAx+PAy*BAy) / ( BAx*BAx + BAy * BAy );
					// T = 0 for cx, 1 for segCX
					//if( t < 0 ) t = 0;
					//if( t > 1 ) t = 1;
					
					if( t > 0 && t < 1 )
					{
						cx = (BAx*t) + cx;
						cy = (BAy*t) + cy;
						//printf( "%f\n", t );
						DFLT	dx = cx - tx;
						DFLT	dy = cy - ty;
						DFLT	dist = sqrtf( dx * dx + dy * dy );
							
							// Makes it so we aren't just intersecting lines, but the conic section
							// produced by the two lines.
						//cor = 0; // ??? I don't know what I was thinking.
						//cor = cor + (segCOR-cor)*t;
						DFLT	dforce = (crad+cor) - dist;
								
								

						if( dforce > 0 )
						{
							if( dist < closestPts && ((cc-e)*(cc-e)) > 25 ) { closestPts = dist; closestAt = cc; }
							thits++;

							tchecks++;
							dforce /= (crad+cor);

							DFLT extraForce = 1.0;
							if( elems[cc].fixed ) extraForce *= 12.0;

							dforce *= dforce * elems[cc].applyForce * sphForceCoeff * extraForce * inter_force_multiplier;

							if( dforce > sphForceMax )
								dforce = sphForceMax;

							if( dist < 0.0001 ) dist = 0.0001;
							DFLT px = -(dx / dist) * dforce;
							DFLT py = -(dy / dist) * dforce;

							elems[e].fx += px;
							elems[e].fy += py;
						}
					}
				}

				if( dforce > 0 )
				{
					if( dist < closestPts && ((cc-e)*(cc-e)) > 25 ) { closestPts = dist; closestAt = cc; }
					thits++;

					tchecks++;
					dforce /= (crad+cor);

					DFLT extraForce = 1.0;
					if( elems[cc].fixed ) extraForce *= 12.0;

					dforce *= dforce * elems[cc].applyForce * sphForceCoeff * extraForce * inter_force_multiplier;

					if( dforce > sphForceMax )
						dforce = sphForceMax;

					if( dist < 0.0001 ) dist = 0.0001;
					DFLT px = -(dx / dist) * dforce;
					DFLT py = -(dy / dist) * dforce;

					elems[e].fx += px;
					elems[e].fy += py;
				}
				
				if( skipnext ) c++;
			}
		}
}

void * execthread( void * v )
{
	intptr_t ip = (intptr_t)v;
	int start = starts[ip];
	int end = ends[ip];
	int worryAbout[WORRY_ABOUT_LEN];

	while( !doquit )
	{
		OGLockSema( threadstarts[ip] );
		execthread_p1(start, end);
		OGUnlockSema( threaddones[ip] );

		OGLockSema( threadstarts[ip] );
		execthread_p2(start, end, worryAbout);
		OGUnlockSema( threaddones[ip] );
	}
	return 0;
}


struct AdhesGraph
{
	DFLT x1, y1;
	DFLT x2, y2;
	int m1;
	int m2;
	int p1;
	int p2;
};
struct AdhesGraph * adhes = 0;
int numAdhesGraph = 0;
int firstAdhesGraph = -1;
int lastAdhesGraph = -1;

DFLT Dist2( DFLT x, DFLT y, DFLT x1, DFLT y1 )
{
	DFLT delX = x - x1;
	DFLT delY = y - y1;
	return sqrt( delX * delX + delY * delY );
}

void AssembleAdhesiveGraph( int e )
{
	struct AdhesGraph * a = adhes + e;

	if( a->m2 < 0 )
	{
		// Try to connect "next"
		float x = a->x2;
		float y = a->y2;
		for( int i = 0; i < numAdhesGraph; i++ )
		{
			if( i == e ) continue;
			struct AdhesGraph * b = adhes + i;
			if( b->m1 >= 0 ) continue;

			if( Dist2( x, y, b->x1, b->y1 ) < 0.1 )
			{
				// We have a match.
				a->m2 = i;
				b->m1 = e;
				AssembleAdhesiveGraph( i );
				break;
			}
			else if( Dist2( x, y, b->x2, b->y2 ) < 0.1 )
			{
				// We have a match, but need to flip.
				DFLT tx = b->x1; b->x1 = b->x2; b->x2 = tx;
				DFLT ty = b->y1; b->y1 = b->y2; b->y2 = ty;
				int  mt = b->m1; b->m1 = b->m2; b->m2 = mt;
				b->m1 = e;
				a->m2 = i;
				AssembleAdhesiveGraph( i );
				break;
			}
		}
	}
	if( a->m1 < 0 )
	{
		// Try to connect "prev"
		float x = a->x1;
		float y = a->y1;

		for( int i = 0; i < numAdhesGraph; i++ )
		{
			if( i == e ) continue;
			struct AdhesGraph * b = adhes + i;
			if( b->m2 >= 0 ) continue;
			if( Dist2( x, y, b->x2, b->y2 ) < 0.1 )
			{
				// We have a match.
				a->m1 = i;
				b->m2 = e;
				AssembleAdhesiveGraph( i );
				break;
			}
			else if( Dist2( x, y, b->x1, b->y1 ) < 0.1 )
			{
				// We have a match, but need to flip.
				DFLT tx = b->x1; b->x1 = b->x2; b->x2 = tx;
				DFLT ty = b->y1; b->y1 = b->y2; b->y2 = ty;
				int  mt = b->m1; b->m1 = b->m2; b->m2 = mt;
				b->m2 = e;
				a->m1 = i;
				AssembleAdhesiveGraph( i );
				break;
			}
		}
	}
}

int netcount = 0;
char ** netlist = 0;

void PossiblyAppendNet( char * netName )
{
	int i;
	for( i = netcount-1; i >= 0; i-- )
	{
		if( strcmp( netlist[i], netName ) == 0 )
			break;
	}

	if( i < 0 )
	{
		netcount++;
		netlist = realloc( netlist, sizeof( char* ) * netcount );
		netlist[netcount-1] = netName;
	}
}



int main()
{
	int i = 0;
	int e;

	ends = malloc( sizeof( int ) * threadct );
	starts = malloc( sizeof( int ) * threadct );
	threads = malloc( sizeof( og_thread_t ) * threadct );
	threadstarts = malloc( sizeof( og_sema_t ) * threadct );
	threaddones = malloc( sizeof( og_sema_t ) * threadct );

	for( int x = 0; x < W; x++ )
	for( int y = 0; y < H; y++ )
	{
		float barrier = 0.0;
		if( x < 9 || y < 9 || x >= W-9 || y >= H-9 ) barrier = 3.0;
		barrierMap[x][y] = barrier;
	}

	int availElem = 0;

	{
		FILE * f = fopen( "../keybase/keybase.kicad_pcb", "r" );
		if( !f )
		{
			fprintf( stderr, "Error: can't open pcb file\n" );
			exit( -6 );
		}
		fseek( f, 0, SEEK_END );
		size_t kicad_len = ftell( f );
		uint8_t * kicadfile_s = malloc( kicad_len );
		fseek( f, 0, SEEK_SET );
		if( fread( kicadfile_s, kicad_len, 1, f ) != 1 )
		{
			fprintf( stderr, "Error: can't read kicad file\n" );
			exit( -5 );
		}
		fclose( f );
		int lineno = 1;
		int c = 0;
		kicad_file = ParseKicadFile( kicadfile_s, kicad_len, &c, &lineno, 0 );
		if( !kicad_file )
		{
			fprintf( stderr, "Parsing failed on %d\n", lineno );
			return -5;
		}

		DFLT minx = 1e20;
		DFLT maxx =-1e20;
		DFLT miny = 1e20;
		DFLT maxy =-1e20;
		int i;

		struct PinnedPads
		{
			DFLT x, y, r;
		};
		struct PinnedPads * pinnedPads = 0;
		int numPinnedPads = 0;
		for( i = 0; i < kicad_file->branchCount; i++ )
		{
			kicadElement * b = kicad_file->branches[i];

			if( strcmp( b->name, "gr_line" ) == 0 )
			{
				char * layerName = KIBR( b, "layer" )->leaves[0];
				int is_EdgeCut = strcmp( layerName, "\"Edge.Cuts\"" ) == 0;
				int is_Adhes = strcmp( layerName, ADHESIVEUSE ) == 0;
				if( !is_EdgeCut && !is_Adhes ) continue;

				DFLT sX, sY, eX, eY;
				for( int j = 0; j < b->branchCount; j++ )
				{
					kicadElement * c = b->branches[j];
					if( strcmp( c->name, "start" ) == 0 || strcmp( c->name, "end" ) == 0 )
					{
						double x = atof( c->leaves[0] );
						double y = atof( c->leaves[1] );
						if( strcmp( c->name, "start" ) == 0 ) { sX = x; sY = y; }
						if( strcmp( c->name, "end" ) == 0 ) { eX = x; eY = y; }
						if( x < minx ) minx = x;
						if( x > maxx ) maxx = x;
						if( y < miny ) miny = y;
						if( y > maxy ) maxy = y;
					}
				}

				if( is_Adhes )
				{
					int ag = numAdhesGraph++;
					adhes = realloc( adhes, sizeof( struct AdhesGraph ) * numAdhesGraph );
					struct AdhesGraph * a = adhes + ag;
					a->x1 = sX;
					a->y1 = sY;
					a->x2 = eX;
					a->y2 = eY;
					a->m1 = -1;
					a->m2 = -1;
					a->p1 = -1;
					a->p2 = -1;
				}
			}

			if( strcmp( b->name, "segment" ) == 0 )
			{
				kicadElement * net = KIBR( b, "net" );
				if( net && net->leafCount > 0 )
				{
					PossiblyAppendNet( net->leaves[0] );
				}
			}

			if( strcmp( b->name, "footprint" ) == 0 )
			{
				for( int j = 0; j < b->branchCount; j++ )
				{
					kicadElement * c = b->branches[j];
					if( strcmp( c->name, "pad" ) == 0 )
					{
						kicadElement * net = KIBR( c, "net" );
						if( net && net->leafCount > 0 )
						{
							PossiblyAppendNet( net->leaves[0] );
						}
					}
				}
			}
		}

		printf( "Net Count %d\n", netcount );

		if( (maxx-minx+2)*10 > W || (maxy-miny+2)*10 > H )
		{
			fprintf( stderr, "Error: W/H aren't big enough (%f,%f)\n", (maxx-minx), (maxy-miny) );
			return -5;
		}


		// Assemble the adhesivegraph.
		{
			printf( "Adhesive graph size: %d\n", numAdhesGraph );
			AssembleAdhesiveGraph( 0 );
			int i;
			for( i = 0; i < numAdhesGraph; i++ )
			{
				if( adhes[i].m1 == -1 )
				{
					if( firstAdhesGraph != -1 ) { fprintf( stderr, "Error: Adhesive Line is Broken\n" ); exit( -6 ); }
					firstAdhesGraph = i;
				}
				if( adhes[i].m2 == -1 )
				{
					if( lastAdhesGraph != -1 ) { fprintf( stderr, "Error: Adhesive Line is Broken\n" ); exit( -6 ); }
					lastAdhesGraph = i;
				}
			}
		}

		//printf( "Extents: %f,%f -> %f,%f\n", minx, miny, maxx, maxy );

		loadOffsetX = minx-1;
		loadOffsetY = miny-1;

		for( i = 0; i < kicad_file->branchCount; i++ )
		{
			kicadElement * b = kicad_file->branches[i];
			if( strcmp( b->name, "gr_line" ) == 0 || strcmp( b->name, "segment" ) == 0 )
			{
				char * layerName = KIBR( b, "layer" )->leaves[0];
				int is_EdgeCut = strcmp( layerName, "\"Edge.Cuts\"" ) == 0;
				int is_Copper = strcmp( layerName, USECOPPER ) == 0;
				int is_UserEco1 = strcmp( layerName, "\"Eco1.User\"" ) == 0;
				
				if( !is_EdgeCut && !is_Copper && !is_UserEco1 ) continue;

				DFLT startX, startY, endX, endY;

				kicadElement * start = KIBR( b, "start" );
				kicadElement * end = KIBR( b, "end" );
				
				startX = atof( start->leaves[0] ) - loadOffsetX;
				startY = atof( start->leaves[1] ) - loadOffsetY;
				endX = atof( end->leaves[0] ) - loadOffsetX;
				endY = atof( end->leaves[1] ) - loadOffsetY;

				startX *= 10;
				startY *= 10;
				endX *= 10;
				endY *= 10;

				DFLT dx = endX - startX;
				DFLT dy = endY - startY;
				if( fabs(dx) > fabs(dy) )
				{
					DFLT ldy = dy/fabs(dx);
					if( startX > endX ) { DFLT tmp = startX; startX = endX; endX = tmp; tmp = startY; startY = endY; endY = tmp; ldy = -ldy; }

					DFLT y = startY;
					for( DFLT x = startX; x <= endX+0.99; x++ )
					{
						barrierMap[(int)x][(int)y] = 3;
						y += ldy;
					}
				}
				else
				{
					DFLT ldx = dx/fabs(dy);
					if( startY > endY ) { DFLT tmp = startY; startY = endY; endY = tmp; tmp = startX; startX = endX; endX = tmp; ldx = -ldx; }
					DFLT x = startX;
					for( DFLT y = startY; y <= endY+0.99; y++ )
					{
						barrierMap[(int)x][(int)y] = 3;
						x += ldx;
					}
				}
			}
			if( strcmp( b->name, "gr_arc" ) == 0 )
			{
				int is_EdgeCut = strcmp( KIBR( b, "layer" )->leaves[0], "\"Edge.Cuts\"" ) == 0;

				if( !is_EdgeCut ) continue;

				DFLT startX, startY, endX, endY, midX, midY;

				kicadElement * start = KIBR( b, "start" );
				kicadElement * mid = KIBR( b, "mid" );
				kicadElement * end = KIBR( b, "end" );

				startX = atof( start->leaves[0] ) - loadOffsetX;
				startY = atof( start->leaves[1] ) - loadOffsetY;

				midX = atof( mid->leaves[0] ) - loadOffsetX;
				midY = atof( mid->leaves[1] ) - loadOffsetY;

				endX = atof( end->leaves[0] ) - loadOffsetX;
				endY = atof( end->leaves[1] ) - loadOffsetY;

				startX *= 10;
				startY *= 10;
				endX *= 10;
				endY *= 10;
				midX *= 10;
				midY *= 10;

				float centerX, centerY;


				float offset = startX*startX + startY*startY;
				float bc =   ( midX*midX + midY*midY - offset )/2.0;
				float cd =   (offset - endX*endX - endY*endY)/2.0;
				float det =  (midX - startX) * (startY - endY) - (startX - endX)* (midY - startY); 

				float idet = 1/det;
				 
				centerX =  (bc * (startY - endY) - cd * (midY - startY)) * idet;
				centerY =  (cd * (midX - startX) - bc * (startX - endX)) * idet;

				float dstartX = startX-centerX;
				float dstartY = startY-centerY;
				float dendX = endX-centerX;
				float dendY = endY-centerY;
				float omegaStart = atan2( dstartX, dstartY );
				float omegaEnd = atan2( dendX, dendY );
				float startmag = sqrtf( dstartX * dstartX + dstartY * dstartY );
				float endmag = sqrtf( dendX * dendX + dendY * dendY );
				float omegaDelta = 1.0/(startmag+endmag);

				float tmp = omegaStart;
				omegaStart = omegaEnd;
				omegaEnd = tmp;

				if( omegaEnd < omegaStart ) omegaEnd += 3.1415926 * 2.0;

				float omega;
				int final = 0;
				for( omega = omegaStart; omega <= omegaEnd; omega+=omegaDelta )
				{
					vst:
					float percent = omega / (omegaEnd-omegaStart);
					float x = sin( omega ) * startmag + centerX;
					float y = cos( omega ) * startmag + centerY;

					barrierMap[(int)x][(int)y] = 3;
					if( final ) break;
					if( omega + omegaDelta > omegaEnd ) { omega = omegaEnd; final = 1; goto vst; }
				}

			}

			if( strcmp( b->name, "via" ) == 0 )
			{
				char * netname = 0;
				int hasflag = 0;
				DFLT px, py, pd;
				
				kicadElement * net = KIBR( b, "net" );
				kicadElement * at = KIBR( b, "at" );
				kicadElement * size = KIBR( b, "size" );

				if( net && net->leafCount > 0 )
				{
					netname = net->leaves[0];
				}
				
				if( at )
				{
					px = atof( at->leaves[0] );
					py = atof( at->leaves[1] );
					hasflag |= 1;
				}

				if( size )
				{
					pd = atof( size->leaves[0] );
					hasflag |= 2;
				}

				if( hasflag == 3 && netname && strcmp( netname, USENET ) == 0 )
				{
					int np = numPinnedPads++;
					pinnedPads = realloc( pinnedPads, numPinnedPads * sizeof( struct PinnedPads ) );
					struct PinnedPads * p = &pinnedPads[np];
					p->x = ( px - loadOffsetX ) * 10;
					p->y = ( py - loadOffsetY ) * 10;
					p->r = pd * 10 / 2 + 1.2; // Need vias to have a little extra wiggle room.
				}
				else if( hasflag == 3 )
				{
					px = ( px - loadOffsetX ) * 10;
					py = ( py - loadOffsetY ) * 10;

					pd *= 10 / 2 + 1.2;
					float ox = px;
					float oy = py;
					int lx, ly;
					for( lx = ox - pd; lx <= ox + pd; lx++ )
					for( ly = oy - pd; ly <= oy + pd; ly++ )
					{
						// Is this right?
						DFLT dx = (lx - px) / pd;
						DFLT dy = (ly - py) / pd;
						DFLT r = sqrt( dx*dx+dy*dy);
						if( r < 1 )
						{
							barrierMap[(int)lx][(int)ly] = 3;
						}
					}
				}
			}

			if( strcmp( b->name, "footprint" ) == 0 )
			{
				DFLT x, y, r = 0;
				int fpgotat = 0;
				
				kicadElement * at = KIBR( b, "at" );

				if( at )
				{
					//printf( "ATLEAF: %d\n", c->leafCount );
					x = atof( at->leaves[0] );
					y = atof( at->leaves[1] );
					if( at->leafCount > 2 ) r = atof( at->leaves[2] ) * 3.14159 / 180.0;
					fpgotat = 1;
				}

				if( !fpgotat ) continue;

				for( int j = 0; j < b->branchCount; j++ )
				{
					kicadElement * c = b->branches[j];

					if( strcmp( c->name, "pad" ) == 0 )
					{
						kicadElement * net = KIBR( c, "net" );
						char * netname = 0 ;

						if( net && net->leafCount > 0 )
						{
							netname = net->leaves[0];
						}

						int onmylayer = 0;
						kicadElement * layers = KIBR( c, "layers" );
						if( layers )
						{
							for( int l = 0; l < layers->leafCount; l++ )
							{
								if( strcmp( layers->leaves[l], USECOPPER ) == 0 ) onmylayer = 1;
								if( strcmp( layers->leaves[l], "\"*.Cu\"" ) == 0 ) onmylayer = 1;
							}
						}

						if( c->leafCount > 2 && onmylayer )
						{
							DFLT px, py, pr;
							DFLT pw, ph;
							DFLT prrr = 0;
							int hasflag = 0;
							
							kicadElement * size = KIBR( c, "size" );
							kicadElement * at = KIBR( c, "at" );
							kicadElement * roundrect_rratio = KIBR( c, "roundrect_rratio" );

							if( at )
							{
								px = atof( at->leaves[0] );
								py = atof( at->leaves[1] );
								pr = atof( at->leaves[2] ) * 3.14159 / 180.0;
								hasflag |= 1;
							}								
							
							if( size )
							{
								pw = atof( size->leaves[0] );
								ph = atof( size->leaves[1] );
								hasflag |= 2;
							}

							if( roundrect_rratio )
							{
								prrr = atof( roundrect_rratio->leaves[0] );
								hasflag |= 4;
							}

							if( netname && strcmp( netname, USENET ) == 0 )
							{
								float compr = sqrt( pw*pw+ph*ph ) * (roundrect_rratio ? 1.2 : 1.0 );// - ((pw+ph)/4.0*prrr); (No benefit from rounded rect), actually it's worse.

								int np = numPinnedPads++;
								pinnedPads = realloc( pinnedPads, numPinnedPads * sizeof( struct PinnedPads ) );
								struct PinnedPads * p = &pinnedPads[np];
								float ox = ( px * cos( r ) + py * sin( r ) ) + x;
								float oy = (-px * sin( r ) + py * cos( r ) ) + y;
								p->x = ( ox - loadOffsetX ) * 10;
								p->y = ( oy - loadOffsetY ) * 10;
								p->r = compr * 10 / 2;
							}
							if( strcmp( c->leaves[1], "np_thru_hole" ) == 0 || ( netname && strcmp( netname, USENET ) != 0 ) )
							{
								if( strcmp( c->leaves[2], "circle" ) == 0 || strcmp( c->leaves[2], "roundrect" ) == 0 )
								{
									if( ( hasflag & 3 ) == 3 )
									{
										float ox = ( px * cos( r ) + py * sin( r ) ) + x;
										float oy = (-px * sin( r ) + py * cos( r ) ) + y;
										float sw = ( pw * cos( r + pr ) + ph * sin( r + pr ) );
										float sh = (-pw * sin( r + pr ) + ph * cos( r + pr ) );
										pr += r;

										sw = fabsf(sw);
										sh = fabsf(sh);

										ox = ( ox - loadOffsetX ) * 10;
										oy = ( oy - loadOffsetY ) * 10;

										sw *= 10 / 2;
										sh *= 10 / 2;
										int lx, ly;
										for( lx = ox - sw; lx <= ox + sw; lx++ )
										for( ly = oy - sh; ly <= oy + sh; ly++ )
										{
											// Is this right?
											DFLT dx = (lx - ox) / sw;
											DFLT dy = (ly - oy) / sh;
											DFLT r = sqrt( dx*dx+dy*dy);
											if( r < 1 )
											{
												barrierMap[(int)lx][(int)ly] = 3;
											}
										}


									}
								}
							}
						}
					}
				}
			}
		}

		{
			int pins = 0;

			printf( "Line Path: " );
			// Pin adhesive group lines to pads.
			for( int i = firstAdhesGraph; i != -1; i = adhes[i].m2 )
			{
				adhes[i].x1 = (adhes[i].x1-loadOffsetX)*10;
				adhes[i].y1 = (adhes[i].y1-loadOffsetY)*10;
				adhes[i].x2 = (adhes[i].x2-loadOffsetX)*10;
				adhes[i].y2 = (adhes[i].y2-loadOffsetY)*10;

				printf( " %d", i );
				DFLT x1 = adhes[i].x1;
				DFLT y1 = adhes[i].y1;
				DFLT x2 = adhes[i].x2;
				DFLT y2 = adhes[i].y2;

				int p;
				for( p = 0; p < numPinnedPads; p++ )
				{
					//printf( "%d -- %f %f %f %f  %f < %f\n", p,  x1, y1, pinnedPads[p].x, pinnedPads[p].y,
					//	Dist2( x1, y1, pinnedPads[p].x, pinnedPads[p].y ), pinnedPads[p].r );
					if( Dist2( x1, y1, pinnedPads[p].x, pinnedPads[p].y ) < pinnedPads[p].r/2 )
					{
						adhes[i].p1 = p;
						printf("(%d-%f)", p, pinnedPads[p].r );
						pins++;
					}
					if( Dist2( x2, y2, pinnedPads[p].x, pinnedPads[p].y ) < pinnedPads[p].r/2 )
					{
						adhes[i].p2 = p;
						printf("[%d-%f]", p, pinnedPads[p].r );
					}
				}
			}
			printf( "\n" );

			if( adhes[firstAdhesGraph].p1 < 0 || adhes[lastAdhesGraph].p2 < 0 )
			{
				fprintf( stderr, "Error: First and last parts of segment must be tied down with a pad or via\n" );
				exit( -16 );
			}
			pins++; // last pin is required.

			printf( "Total Pins: %d\n", pins );

			double pointsPerGroup = (ELEMENTS-pins) / (double)(pins-1);
			printf( "PPG: %f\n", pointsPerGroup );

			int i;
			int group;
			int a = firstAdhesGraph;
			int e = 0;
			int ap;
			double eprog = 0;
			for( i = 0; i < pins-1; i++ )
			{
				float grouplen = 0.0;
				grouplen = -pinnedPads[adhes[a].p1].r*1.3;
				int astart = a;

				ap = a;
				for( ; adhes[ap].p2 < 0; ap = a, a = adhes[a].m2 )
				{
					grouplen += Dist2( adhes[a].x1, adhes[a].y1, adhes[a].x2, adhes[a].y2 );
				}
				grouplen -= pinnedPads[adhes[ap].p2].r*1.3;
				printf( "Group Len: %f\n", grouplen );

				a = astart;

				DFLT pgr = grouplen / pointsPerGroup;

				// Now, from astart actually start putting down some points.
				eprog += (pointsPerGroup+1);
				ap = a;
				for( ; adhes[ap].p2 < 0; ap = a, a = adhes[a].m2 )
				{
					DFLT Cx = adhes[a].x1;
					DFLT Cy = adhes[a].y1;
					DFLT Dx = adhes[a].x2 - adhes[a].x1;
					DFLT Dy = adhes[a].y2 - adhes[a].y1;
					DFLT Dmag = sqrt( Dx*Dx + Dy*Dy );
					Dx /= Dmag;
					Dy /= Dmag;

					DFLT dProg = 0;
					if( adhes[a].p1 >= 0 ) dProg += pinnedPads[adhes[a].p1].r*1.3;

					// Clear the source.
					Cx += dProg * Dx;
					Cy += dProg * Dy;

					if( adhes[a].p1 >= 0 )
					{
						elems[e].fixed = 1;
						elems[e].finalRadius = elems[e].currentRadius = pinnedPads[adhes[a].p1].r;
						elems[e].radSpeed = radSpeed*2;
						elems[e].applyForce = 3;
						elems[e].x = pinnedPads[adhes[a].p1].x;
						elems[e].y = pinnedPads[adhes[a].p1].y;
						e++;
					}

					while( e < eprog-1 && dProg <= Dmag )
					{
						//grouplen
						elems[e].fixed = 0;
						elems[e].x = Cx + (rand()%1000)/10000.0;
						elems[e].y = Cy + (rand()%1000)/10000.0;
						elems[e].currentRadius = pgr;
						elems[e].radSpeed = radSpeed;
						elems[e].applyForce = 1;
						elems[e].finalRadius = targetRadius;
						Cx += Dx*pgr;
						Cy += Dy*pgr;
						dProg += pgr;
						e++;
					}
					printf( "%d / %f\n", e, eprog );
					//printf( "... %d\n", a );
					//grouplen += Dist2( adhes[a].x1, adhes[a].y1, adhes[a].x2, adhes[a].y2 );
				}
				//grouplen -= pinnedPads[adhes[ap].p2].r;
				//printf( "Group (ending %d): Len: %f\n", a, grouplen );

				a = adhes[ap].m2;
			}


			// Potentially fixup janky math.
			mulligan:

			elems[e].fixed = 1;
			elems[e].finalRadius = elems[e].currentRadius = pinnedPads[adhes[ap].p2].r;
			elems[e].radSpeed = radSpeed*2;
			elems[e].applyForce = 2;
			elems[e].x = pinnedPads[adhes[ap].p2].x;
			elems[e].y = pinnedPads[adhes[ap].p2].y;

			if( e == ELEMENTS-2 )
			{
				// Make up for it.

				e++; goto mulligan;
			}

			if( e != ELEMENTS-1 )
			{
				fprintf( stderr, "Error: Math didn't math.  Got %d.  Expected %d\n", e, ELEMENTS-1 );
				exit( -5 );
			}

		}

	}

	for( i = 0; i < 4; i++ )
	{
		for( int x = 0; x < W; x++ )
		for( int y = 0; y < H; y++ )
		{
			float flmax = barrierMap[x][y];
			for( int sx = x-1; sx <= x+1; sx++ )
			for( int sy = y-1; sy <= y+1; sy++ )
			{
				if( sx < 0 || sx >= W || sy < 0 || sy >= H ) continue;

				DFLT bm = barrierMap[sx][sy] - 1.0;
				if( bm > flmax ) flmax = bm;
			}
			barrierMap[x][y] = flmax;
		}
	}


	// Build the primary path
	{
		int rp;
		
	}


	{
		DFLT et = ELEMENTS / (DFLT)threadct;
		for( e = 0; e < threadct; e++ )
		{
			starts[e] = e * et;
			ends[e] = (e+1)*et;
			threadstarts[e] = OGCreateSema();
			threaddones[e] = OGCreateSema();
			threads[e] = OGCreateThread( execthread, (void*)(intptr_t)e );
		}
		ends[threadct-1] = ELEMENTS;
	}

	#define NRBP 24
	DFLT pointsBase[NRBP][2];// = { { -5, 36}, {20, 50}, { 40, 50 } };
	for( i = 0; i < NRBP; i++ )
	{
		pointsBase[i][0] = sin( i / (NRBP/6.2831852) );
		pointsBase[i][1] = cos( i / (NRBP/6.2831852) );
	}


	CNFGSetup( "SerpentineGen", 1920, 1080 );

	// barrierMap is assumed to not change anymore, so we can pre-convert and upload as a texture to save time not rendering 1M+ polygons every frame
	for (int x = 0; x < W; x++)
	for (int y = 0; y < H; y++)
	{
		float BarrierVal = (float)barrierMap[x][y] * 20.0F;
		uint8_t GrayVal = (uint8_t)round(BarrierVal);
		int ColourHere = (BarrierVal < 2.0F) ? 0 : (0xFF | (GrayVal << 8) | (GrayVal << 16) | (GrayVal << 24));
		barrierMapTextureData[(y * W) + x] = ColourHere;
	}
	unsigned int BarrierMapTextureID = CNFGTexImage(barrierMapTextureData, W, H);

	CNFGSetLineWidth( 2 );
	while(CNFGHandleInput())
	{
		CNFGBGColor = 0x000020ff; //Dark Blue Background

		CNFGGetDimensions( &cww, &cwh );
		
		DFLT fPercentToFull = ( frameno / (DFLT)maxRadSpeedSpeedup );
		if( fPercentToFull > 1.0 ) fPercentToFull = 1.0;
		DFLT springForce = springForceStart + (springForceFinal-springForceStart)*fPercentToFull;
		sphForceCoeff = sphForceCoeffStart + (sphForceCoeffFinal-sphForceCoeffStart)*fPercentToFull;

		CNFGClearFrame();
	
		int e;

		double dPhase1 = OGGetAbsoluteTime();

		// Step 1: Put all elements into the data structure to help find them.
		for( int t = 0; t < threadct; t++ )
			OGUnlockSema( threadstarts[t] );

		for( int t = 0; t < threadct; t++ )
			OGLockSema( threaddones[t] );

		double dPhase2 = OGGetAbsoluteTime();

		// Step 2: Apply forces from nearby elements.
		// This is expensive, so do this in threads.
		for( int t = 0; t < threadct; t++ )
			OGUnlockSema( threadstarts[t] );

		for( int t = 0; t < threadct; t++ )
			OGLockSema( threaddones[t] );
		double dPhase3 = OGGetAbsoluteTime();

		// Step 3: Apply forces from chain.
		for( e = 0; e < ELEMENTS; e++ )
		{
			DFLT tx = elems[e].x;
			DFLT ty = elems[e].y;

			DFLT fx = elems[e].fx;
			DFLT fy = elems[e].fy;

			DFLT cr = elems[e].currentRadius * tetherShrink;

			if( elems[e].fixed ) continue;

			// Apply spring force
			for( int neighbor = e - 1; neighbor <= e + 1; neighbor++ )
			{
				if( neighbor == e ) continue;
				DFLT nx = elems[neighbor].x;
				DFLT ny = elems[neighbor].y;

				DFLT dx = nx - tx;
				DFLT dy = ny - ty;

				DFLT dist = sqrtf( dx * dx + dy * dy );
				dx /= dist;
				dy /= dist;
				DFLT dpush = (dist - cr) * springForce * springForceUserMux;

				fx += dx * dpush;
				fy += dy * dpush;
			}

			DFLT vx = elems[e].vx = elems[e].vx * velocityDamp + fx * ( dt * !paused );
			DFLT vy = elems[e].vy = elems[e].vy * velocityDamp + fy * ( dt * !paused );

			elems[e].fx = fx + vx;
			elems[e].fy = fy + vy;
		}
		double dPhase4 = OGGetAbsoluteTime();

		// Actually move
		for( e = 0; e < ELEMENTS; e++ )
		{
			DFLT tx = elems[e].x;
			DFLT ty = elems[e].y;
			DFLT fx = elems[e].fx;
			DFLT fy = elems[e].fy;

			elems[e].fx = 0;
			elems[e].fy = 0;

			// Ignore NaNs
			if( fx != fx || fy != fy ) continue;
			if( elems[e].fixed ) continue;
			if( elems[e].pinned || ( e == grabbede && grabdown ) ) continue;

			elems[e].x = tx + (dt * !paused) * fx;
			elems[e].y = ty + (dt * !paused) * fy;

		}

		// Bring radius to be more in inline with target radius;
		for( e = 0; e < ELEMENTS; e++ )
		{
			DFLT cr = elems[e].currentRadius;
			DFLT dr = elems[e].finalRadius * ( elems[e].fixed ? 1.0 : sizeUserMux ) - cr;
			DFLT radspeed = elems[e].radSpeed;

			int radspeedmux = frameno;
			if( radspeedmux > maxRadSpeedSpeedup ) radspeedmux = maxRadSpeedSpeedup;
			radspeed *= radspeedmux * speedUserMux;
			if( elems[e].fixed ) radspeed *= 2;
			if( dr < -radspeed ) dr = -radspeed;
			if( dr > radspeed ) dr = radspeed;
			elems[e].currentRadius += dr;
		}
		double dPhase5 = OGGetAbsoluteTime();

		CNFGBlitTex(BarrierMapTextureID, -0.5 * zoom + xofs, -0.5 * zoom + yofs, W * zoom, H * zoom);

		for( e = 0; e < ELEMENTS; e++ )
		{
			DFLT tx = elems[e].x;
			DFLT ty = elems[e].y;

			DFLT cx = tx * zoom + xofs;
			DFLT cy = ty * zoom + yofs;

			if( cx < -16384 || cx > 16384 || cy < -16384 || cy > 16384  ) continue;

			if( e == closestAt )
			{
				CNFGColor( 0xff000080 ); 
			}
			else if( elems[e].fixed )
			{
				CNFGColor( 0xffffff80 ); 
			}
			else if( e == grabbede )
			{
				CNFGColor( 0x0000ff80 ); 
			}
			else if( elems[e].pinned )
			{
				CNFGColor( 0x00000080 ); 
			}
			else
			{
				if (!renderBubbles) continue;
				CNFGColor( 0xffffff10 ); 
			}

			RDPoint points[NRBP];

			DFLT cr = elems[e].currentRadius;

			int i;

			for( i = 0; i < NRBP; i++ )
			{
				points[i].x = (pointsBase[i][0] * cr + tx) * zoom + xofs;
				points[i].y = (pointsBase[i][1] * cr + ty) * zoom + yofs;
			}

			CNFGTackPoly( points, NRBP ); // EXTREMELY EXPENSIVE!!
		}

		//Change color to white.
		CNFGColor( 0xffffff80 ); 


		DFLT totalLen = 0;

		for( e = 0; e < ELEMENTS-1; e++ )
		{
			int n = e+1;
			DFLT ax = elems[e].x * zoom + xofs;
			DFLT ay = elems[e].y * zoom + yofs;
			DFLT bx = elems[n].x * zoom + xofs;
			DFLT by = elems[n].y * zoom + yofs;

			DFLT dx = elems[e].x - elems[n].x;
			DFLT dy = elems[e].y - elems[n].y;
			totalLen += sqrt( dx * dx + dy * dy ) / 10;

			if( ax < -16384 || ax > 16384 || ay < -16384 || ay > 16384 ||
				bx < -16384 || bx > 16384 || by < -16384 || by > 16384 ) continue;
			CNFGTackSegment( ax, ay, bx, by ); // This is quite expensive, could just the data be saved off and then rendered while the next cycle is running?
		}

		CNFGColor( 0xffffffff );

		for (int x = 0; x < EMX; x++)
		{
			// This removes the atomic constraint for clearing.
			// We can guarantee that no other threads are active during this phase, so this is safe as long as that assumption holds.
			// Without this contraint lift, the compiler cannot optimize the array clearing at all, because each element must individually be atomically written.
			// By removing this atomicity constraint, an optimized memset is used, significantly speeding this up (I measured about 1.8ms per frame)
			int* NonAtomicArr = (int*)(elementMapCnt[x]);
			for (int y = 0; y < EMY; y++)
			{
				NonAtomicArr[y] = 0;
			}
		}
		double dPhase6 = OGGetAbsoluteTime();

		float onesize = 1000*zoom;
		float markermargin = 40;
		float permm = 100;
		while( onesize < 1000 )
		{
			permm *= 10;
			onesize *= 10;
		}
		while( onesize > 1000 )
		{
			permm /= 10;
			onesize /= 10;
		}
		CNFGTackSegment( cww-markermargin, cwh-markermargin-20, cww-markermargin, cwh-markermargin );
		int i;
		for( i = 1; i < 10; i++ )
		{
			CNFGTackSegment( cww-markermargin-i*onesize/10, cwh-markermargin-20, cww-markermargin-i*onesize/10, cwh-markermargin-10 );
		}
		CNFGTackSegment( cww-markermargin, cwh-markermargin-10, cww-markermargin-onesize, cwh-markermargin-10 );
		CNFGTackSegment( cww-markermargin-onesize, cwh-markermargin-20, cww-markermargin-onesize, cwh-markermargin );

		char cts[1024];
		int ax, ay;
		sprintf( cts, "%.3fmm", permm );
		CNFGGetTextExtents( cts, &ax, &ay, 3 );
		CNFGPenX = cww-markermargin-onesize/2 - ax/2;
		CNFGPenY = cwh-markermargin-6;
		CNFGDrawText( cts, 3 );

		sprintf( cts, "frame %d\ne3 cr %f\nsphForceCoeff %f\nratiohit %f\nP1: %.3fms\nP2: %.3fms\nP3: %.3fms\nP4: %.3fms\nP5: %.3fms\nTL: %.1fmm\nCL: %.3fmm\nOfs: %.3f,%.3f", frameno, elems[3].currentRadius,sphForceCoeff, thits/(DFLT)ttotal, (dPhase2-dPhase1)*1000.0, (dPhase3-dPhase2)*1000.0, (dPhase4-dPhase3)*1000.0, (dPhase5-dPhase4)*1000.0, (dPhase6-dPhase5)*1000.0, totalLen, closestPts/10.0, loadOffsetX, loadOffsetY );
		closestPts = 1e20;
		thits = 0;
		ttotal = 0;
		CNFGPenX = 3;
		CNFGPenY = 3;
		CNFGDrawText( cts, 3 );

		sprintf( cts, "Force (bubble)  [r][f] = %.2f\nSpring (force)  [t][g] = %.2f\nSizeMux         [y][h] = %.2f\nLine Collision     [l] = %s\nUser Speed Mux  [u][j] = %.2f\nDt              [i][k] = %.3f\nPaused      [SPACEBAR] = %s\nRender Bubbles     [b] = %s\nJump To Closest [z]\n",
			inter_force_multiplier, springForceUserMux, sizeUserMux,
			enable_line_collision ? "Yes" : "No", speedUserMux, dt, paused?"PAUSED":"RUNNING", renderBubbles?"Yes":"No");
		CNFGPenX = 250;
		CNFGPenY = 3;
		CNFGDrawText( cts, 3 );

		//Display the image and wait for time to display next frame.
		CNFGSwapBuffers();
		frameno++;	
	}
	return 0;
}



