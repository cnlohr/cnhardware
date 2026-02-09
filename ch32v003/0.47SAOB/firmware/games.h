#ifndef _GAMES_H
#define _GAMES_H

int totalScore;

uint32_t gameTimeUs;
int gameModeID;
int buttonMask;
int buttonEventDown;
int buttonEventUp;

const int maxGames = 10;
int gameNumber = 0;

void DoneGame( int score );

typedef int (*gamemode_t)();

uint8_t gameData[1152];

const int8_t dX[4] = { -1, 0, 1, 0 };
const int8_t dY[4] = { 0, -1, 0, 1 };

gamemode_t gameMode;


// high score stuff.
void WriteHighScore( int32_t hs );
#ifndef EMU
int32_t * const highScorePtr = (int32_t*)0x08003ffc;
#endif

int GameModeGrand()
{
	struct
	{
		int stage;
		uint32_t endMark;
		uint32_t switchTime;
	} * g = (void*)gameData;

	glyphdraw_invert = -1;
	glyphdraw_nomask = 1;

	uint32_t tsTime = gameTimeUs - g->switchTime;

	switch (g->stage)
	{
	case 0:
		background(11);
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "A GRAND" );
		swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", (gameTimeUs>>12) );
		if( buttonEventDown || (gameTimeUs>>12) >= 2000 )
		{
			g->switchTime = gameTimeUs;
			g->endMark = ((gameTimeUs>>12) >= 2000) ? 2000 : (gameTimeUs>>12);
			g->stage = 1;
		}
		break;
	case 1:
	{
		// Flash
		background(10);
		glyphdraw_invert = 0;
		glyphdraw_nomask = 1;
		int substage = (tsTime>>20);
		int shownum = g->endMark;
		int showdiff = 1000 - shownum;
		int score = 1000 - ((showdiff<0)?-showdiff:showdiff);
		if( score < 0 ) score = 0;
		if( substage == 1 )
		{
			shownum = showdiff;
		}
		else if( substage == 2 )
		{
			shownum = score;
		}
		else if( substage > 2 )
		{
			totalScore += score;
			return 1;
		}
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, (const char*[]){"A GRAND","DIFFERENCE","EARN"}[substage] );
		swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", shownum );
	}
	}

	return 0;
} 

int GameModeTunnel()
{
	int gmid = gameModeID;
	const int tun_end = 500;
	struct
	{
		int stage;
		uint32_t switchTime;
		int t;
		int shipx;
		uint8_t tunnel_pos[500 + 72]; // May only use up to +40
	} * g = (void*)gameData;

	const int tun_wid = gmid ? 40 : 72;
	const int tun_ext = gmid ? 72 : 40;
	const int tun_max = tun_wid-10;
	const int tun_min = gmid ? 5 : 10;
	const int tun_cen = ( tun_max + tun_min ) / 2;

	if( gameTimeUs == 0 )
	{
		int i;
		_rand_lfsr = SysTick->CNT;
		int cp = tun_cen;
		for( i = 0; i < sizeof( g->tunnel_pos ); i++ )
		{
			uint32_t code = _rand_gen_nb(8) & 1;  // Get 8 bits so we get some entropy.
			int delta = 0;
			if( i & 1 )
			{
				if( code == 0 ) delta = -1;
				if( code == 1 ) delta = 1;
			}
			cp += delta;
			if( cp < tun_min ) cp = tun_min;
			if( cp > tun_max ) cp = tun_max;
			g->tunnel_pos[i] = cp;
		}
		g->shipx = tun_cen;
	}
	int i;
	int hit = 0;
	for( i = 0; i < tun_ext; i++ )
	{	
		int tp = g->t+i;
		int width = 0;
		if( gmid )
		{
			width = 13 - (( tp ) >> 6);
		}
		else
		{
			width = 21 - (( tp ) >> 5);
		}
		uint8_t gtp = g->tunnel_pos[tp];
		int c = gtp - width;
		int n = gtp + width;
		//printf( "%d %d\n", width, gtp );
		if( c < 0 ) c = 0;
		if( gmid )
		{
			int y = i;
			ssd1306_drawFastVLine( y, 0, c, 1);
			ssd1306_drawFastVLine( y, c, n, 0);
			ssd1306_drawFastVLine( y, n, tun_wid, 1);
		}
		else
		{
			int y = tun_ext - i - 1;
			ssd1306_drawFastHLine( 0, y, c, 1);
			ssd1306_drawFastHLine( c, y, n, 0);
			ssd1306_drawFastHLine( n, y, tun_wid, 1);
		}
		if( i == 0 )
		{
			int sx = g->shipx;
			if( sx < c || sx > n ) hit = 2;
		}
	}

	if( gmid )
	{
		ssd1306_fillCircle( 1, g->shipx, 3, 1 );
	}
	else
	{
		ssd1306_fillCircle( g->shipx, tun_ext - 1, 3, 1 );
	}

	int st = (gameTimeUs - g->switchTime)>>12;;
	int tmp;

	glyphdraw_invert = ( (st) & 1 ) - 1;
	glyphdraw_nomask = 1;

	switch( g->stage )
	{
	case 0:
		tmp = 400-st;
		if( tmp < 0 ) { tmp = 0; g->stage = 1; g->switchTime = gameTimeUs; }
		swadgeDraw( SSD1306_W, 0, 2, swadgeGlyphHalf, "%d", tmp );
		glyphdraw_invert = 0;
		glyphdraw_nomask = 0;
		swadgeDraw( SSD1306_W/2, 24, 1, swadgeGlyph, gmid?"SLIDE":"SHIFT" );
		break;
	case 1:
		swadgeDraw( SSD1306_W, 0, 2, swadgeGlyphHalf, "%d", g->t*2 );
		int nextt = st>>1;
		if( nextt != g->t && (nextt & 0x3) == 0 )
		{
			if( buttonMask & 1 ) g->shipx--;
			if( buttonMask & 2 ) g->shipx++;
		}
		if( nextt >= tun_end ) { nextt = tun_end; hit = 3; }
		g->t = nextt;
		if( hit ) { g->stage = hit; g->switchTime = gameTimeUs; }
		break;
	case 2:
	case 3:
		if( st > 400 ) { totalScore += g->t*2; return 1; }
		swadgeDraw( SSD1306_W/2, 0, 1, swadgeGlyph, (g->stage == 3)?"RAN":"TUPP" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", g->t*2 );
		break;	
	}
	return 0;
}


int GameModeFlap()
{
	const int marginx = 5;
	struct
	{
		int subMode;
		uint32_t switchTime;
		uint32_t lastTime;
		int score;
		int gplace;
		int birdyHR;
		int passwicket;
		int birdySPD;
		uint8_t wp[10];
	} * g = (void*)gameData;

	int nextSubMode = g->subMode;

	int st = (gameTimeUs - g->switchTime)>>12;
	backgroundAttrib = 512 - (st>>3);
	background( 13 );

	glyphdraw_invert = 0;
	glyphdraw_nomask = 1;

	int birdy = g->birdyHR>>14;


	if( gameTimeUs == 0 )
	{
		int i;
		_rand_lfsr = SysTick->CNT;
		int last = 16;
		for( i = 0; i < sizeof( g->wp ); i++ )
		{
			int code = _rand_gen_nb(8) & 0x1f;  // Get 8 bits so we get some entropy.
			if( code - last > 12 ) code = last + 12;
			if( code - last <-12 ) code = last - 12;
			g->wp[i] = code;
		}
		g->birdyHR = (SSD1306_H/2) << 14;
		g->birdySPD = -500;
	}
	int i;

	for( i = 0; i < sizeof( g->wp ); i++ )
	{
		int wp = g->wp[i] - 1;
		int margin = 14 - i;

		int x = 100 + i * 60 - g->gplace;

		if( x >= SSD1306_W || x < 0 ) continue;

		int minm = wp + 8 - margin;
		if( minm < 0 ) minm = 0;
		int maxm = wp + 8 + margin;
		if( maxm >= SSD1306_H ) maxm = SSD1306_H-1;
		ssd1306_drawRect(x-2, 0, 4, minm, 1);
		ssd1306_drawRect(x-2, maxm, 4, SSD1306_W-1, 1);

		if( nextSubMode == 1 )
		{
			if( x-2 < marginx && x + 2 > marginx )
			{
				int by = birdy;
				if( by < minm || by > maxm )
				{
					nextSubMode = 2;
				}
				else if( g->passwicket < i+1 && x + 2 > marginx )
				{
					g->passwicket = i+1;
					g->score+=100;
				}
			}
		}
	}


	int dt = (gameTimeUs>>10) - g->lastTime;
	g->lastTime = (gameTimeUs>>10);

	switch( g->subMode )
	{
	case 0:
		swadgeDraw( SSD1306_W/2, 22, 1, swadgeGlyph, "FLAP" );
		if( st > 400 )
		{
			nextSubMode = 1;
		}
		break;
	case 1:
	case 2:  // Dead
		swadgeDraw( SSD1306_W, 29, 2, swadgeGlyphHalf, "%d", g->score );

		if( buttonEventDown )
			g->birdySPD = -500;

		// Doing it th is way makes sure we have a stable update rate
		while( dt > 0 )
		{
			g->birdySPD += 1;
			g->birdyHR += g->birdySPD * 1;
			birdy = g->birdyHR >> 14;
			if( g->subMode == 1 )
			{
				g->gplace = st>>2;

				if( 100 + 9 * 60 + 15 < g->gplace )
				{
					nextSubMode = 3;
				}
				if( birdy < -20 || birdy > SSD1306_H + 1 )
				{
					nextSubMode = 2;
				}
			}
			else
			{
				swadgeDraw( SSD1306_W/2, 10, 1, swadgeGlyph, "DEAD" );
				if( st > 400 )
				{
					nextSubMode = 3;
				}
			}
			dt--;
		}
		if( birdy > -1 && birdy <= SSD1306_H )
			ssd1306_fillCircle( marginx, birdy, 3, 1 );
		break;
	case 3:
		if( st > 400 ) 
		{
			totalScore += g->score;
			return 1;
		}
		swadgeDraw( SSD1306_W/2, 0, 1, swadgeGlyph, "FLUP" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", g->score );
		break;	
	}

	if( nextSubMode != g->subMode )
	{
		g->subMode = nextSubMode;
		g->switchTime = gameTimeUs;
	}

	return 0;
}

int GameModeSnek()
{
	#define SS 8
	#define MAXSH 256
	struct
	{
		int subMode;
		uint32_t switchTime;
		int lastst;
		int sx;
		int sy;
		int gd;
		int ax, ay;
		int nextgd;
		uint8_t sh[MAXSH][2];
		int shh;
		int shh_valid;
		int sneklen;
		int score;
	} * g = (void*)gameData;

	int nextSubMode = g->subMode;
	if( gameTimeUs == 0 )
	{
		g->sx = SSD1306_W/2/SS;
		g->sy = SSD1306_H/SS-1;
		g->sneklen = 4;
		g->nextgd = g->gd = 1;
	}

	if( g->ax == 0 )
	{
		int ax, ay;
		do
		{
			ax = rand()%(SSD1306_W/SS-2)+1;
			ay = rand()%(SSD1306_H/SS-2)+1;
			//printf( "%d %d\n", ax, ay );
			if( ax == g->sx && ay == g->sy ) continue;

			int sl = g->sneklen;
			int rl = (uint8_t)(g->shh - g->shh_valid);
			if( sl > rl ) sl = rl;
			int i;
			for( i = 0; i < sl; i++ )
			{
				int gp = (g->shh - 1 - i)&(MAXSH-1);
				int bx = g->sh[gp][0];
				int by = g->sh[gp][1];
				if( bx == ax && by == ay )
				{
					break;
				}
			}
			if( i != sl ) continue;
			break;
		} while( 1 );
		g->ax = ax;
		g->ay = ay;
	}

	background( 0 );

	int st = (gameTimeUs - g->switchTime)>>18;
	int dst = st - g->lastst;

	int i;
	int sl = g->sneklen;
	int rl = (uint8_t)(g->shh - g->shh_valid);
	if( sl > rl ) sl = rl;
	for( i = 0; i < sl; i++ )
	{
		int gp = (g->shh - 1 - i)&(MAXSH-1);
		int bx = g->sh[gp][0];
		int by = g->sh[gp][1];
		ssd1306_drawRect( bx*SS, by*SS, SS, SS, 1 );
		if( i && bx == g->sx && by == g->sy )
		{
			nextSubMode = 3;
		}
	}
	{
		int ssx = g->sx * SS;
		int ssy = g->sy * SS;
		int sdx = dX[g->gd];
		int sdy = dY[g->gd];
		ssd1306_drawRect( ssx, ssy, SS, SS, 1 );
		if( sdx )
	        ssd1306_drawFastVLine( ssx + 2 + (sdx+1), ssy, SS, 1);
		if( sdy )
	        ssd1306_drawFastHLine( ssx, ssy + sdy + 2 + (sdy+1), SS, 1);
	}
	{
		ssd1306_fillCircle( g->ax*SS+SS/2, g->ay*SS+SS/2, 2, 1 );	
	}

	switch( g->subMode )
	{
	case 0:
		swadgeDraw( SSD1306_W/2, 22, 1, swadgeGlyphHalf, "NO STEP" );
		if( st > 5 )
		{
			nextSubMode = 1;
		}
		break;
	case 1:
	{
		//swadgeDraw( SSD1306_W, 0, 2, swadgeGlyphHalf, "%d", g->score );
		if( dst )
		{
			int dx = dX[g->gd];
			int dy = dY[g->gd];
			g->gd = g->nextgd;
			g->sx += dx;
			g->sy += dy;
			g->sh[g->shh][0] = g->sx;
			g->sh[g->shh][1] = g->sy;
			g->shh = (g->shh+1) & ( MAXSH - 1 );

			int rl = (uint8_t)(g->shh - g->shh_valid);
			if( rl > g->sneklen )
			{
				g->shh_valid = (g->shh_valid+1) & (MAXSH - 1);
			}
			if( g->sx < 0 || g->sx >= SSD1306_W/SS || g->sy < 0 || g->sy >= SSD1306_H/SS)
			{
				nextSubMode = 3;
			}

			if( g->sx == g->ax && g->sy == g->ay )
			{
				g->sneklen += 4;
				g->score += 200;
				g->ax = 0;
				g->ay = 0;
				if( g->score >= 1000 )
				{
					nextSubMode = 4;
				}
			}
		}

		if( buttonEventDown & 2 )
			g->nextgd = (g->gd + 1)&3;
		if( buttonEventDown & 1 )
			g->nextgd = (g->gd - 1)&3;
		break;
	case 3:
		glyphdraw_nomask = 1;
		swadgeDraw( SSD1306_W/2, 3, 1, swadgeGlyphHalf, "NOOODLE" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", g->score );
		if( st > 5 )
		{
			totalScore += g->score;
			return 1;
		}
		break;
	case 4:
		glyphdraw_nomask = 1;
		swadgeDraw( SSD1306_W/2, 3, 1, swadgeGlyphHalf, "SNEK" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", g->score );
		if( st > 5 )
		{
			totalScore += g->score;
			return 1;
		}
		break;
	}
	}
	g->lastst = st;
	if( nextSubMode != g->subMode )
	{
		g->subMode = nextSubMode;
		g->switchTime = gameTimeUs;
	}

	return 0;
}

int GameBeatTop()
{
	#define beats 25
	const int scorebar = SSD1306_H - 3;
	struct
	{
		int subMode;
		uint32_t switchTime;
		uint8_t beatpl[beats];
		int tofjudge;
		int lastjudgdir;
		int judge;
		int beatplace;
		int score;
	} * g = (void*)gameData;

	int i;
	int nextSubMode = g->subMode;
	if( gameTimeUs == 0 )
	{
		_rand_lfsr = SysTick->CNT;
		for( i = 0; i < beats; i++ )
		{
			g->beatpl[i] = _rand_gen_nb(1);
		}
		g->judge = -1;
		//g->sx = SSD1306_W/2/SS;
		//g->sy = SSD1306_H/2/SS;
		//g->sneklen = 4;
	}

	background(10);

	glyphdraw_invert = 0;
	glyphdraw_nomask = 1;

	int st = (gameTimeUs - g->switchTime)>>12;

	ssd1306_drawRect(2,scorebar, SSD1306_W-4, 1, 1);

	for( i = 0; i < beats; i++ )
	{
		int b = g->beatpl[i];
		if( b == 2 ) continue;
		int by = SSD1306_H + g->beatplace - 30 - i * 32;
		int bx = SSD1306_W / 2 * b + 2;
		if( by < 0 ) continue;
		if( by >= SSD1306_H + 10 ) continue;
		ssd1306_drawRect( bx + 2, by, SSD1306_W/2-6,2, 1 );
		int byd = by - scorebar;
		if( byd < 0 ) byd = -byd;
		if( buttonEventDown && byd < 16 )
		{
			int judge = 0;
			if(buttonEventDown & 1)
			{
				if( b ) judge = 4;
			}
			if(buttonEventDown & 2)
			{
				if( !b ) judge = 4;
			}
			if( !judge )
			{
				judge = byd / 4;
			}
			g->lastjudgdir = b*2-1;
			g->score += (4-judge)*10;
			g->judge = judge;
			g->tofjudge = gameTimeUs;
			g->beatpl[i] = 2;
			buttonEventDown = 0;
		}
	}

	switch( nextSubMode )
	{
	case 0:
		swadgeDraw( SSD1306_W/2, 22, 1, swadgeGlyph, "BEAT" );
		if( st > 400 )
		{
			nextSubMode = 1;
		}
		break;
	case 1:
		swadgeDraw( SSD1306_W/2, 1, 1, swadgeGlyphHalf, "%d", g->score );
		if( g->judge >= 0 )
		{
			int jdiff = ((gameTimeUs - g->tofjudge)>>13) + 8;
			if( jdiff < 100 )
				swadgeDraw( SSD1306_W/2+g->lastjudgdir*jdiff, 12, 1, swadgeGlyphHalf, (const char*[]){"PERFECT","GREAT","OK","BAD","MISS"}[g->judge] );
		}
		g->beatplace = st>>2;
		if( g->beatplace > 850 )
		{
			nextSubMode = 2;
		}
		break;
	case 2:
		swadgeDraw( SSD1306_W/2, 3, 1, swadgeGlyphHalf, "JAM" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", g->score );

		if( st > 400 )
		{
			totalScore += g->score;
			return 1;
		}
		break;
	}

	if( nextSubMode != g->subMode )
	{
		g->subMode = nextSubMode;
		g->switchTime = gameTimeUs;
	}

	return 0;
}

int RadialGame()
{
	#define RADIIS 10
	struct
	{
		int subMode;
		int switchTime;
		int lastStg;

		int rotation;
		int rotdir;
		int8_t panels[20];

		int8_t consumed[RADIIS];

		const char * alertValue;
		int alertTime;
		int score;
	} * g = (void*)gameData;
	int i;
	int nextSubMode = g->subMode;

	if( gameTimeUs == 0 )
	{
		_rand_lfsr = SysTick->CNT;
		for( i = 0; i < sizeof(g->panels); i++ )
		{
			g->panels[i] = _rand_gen_nb(8)%RADIIS;
		}
		g->rotation = 0;
	}

	glyphdraw_invert = 0;
	glyphdraw_nomask = 1;

	background(0);

	int st = (gameTimeUs - g->switchTime)>>12;
	int stg = st;
	int dstg = stg - g->lastStg;
	g->lastStg = stg;
	if( g->subMode == 0 ) stg = 0;


	int oring = stg>>8;
	int oringdtime = (stg&0xff);
	for( i = 0; i < oring+8; i++ )
	{
		if( i >= sizeof( g->panels ) ) break;
		int8_t p = g->panels[i];
		if( p < 0 ) continue;
		int size = (((oring - i + 2)<<8) + (oringdtime<<0))<<11;
		if( size < 0 ) { ssd1306_drawPixel( 36, 20, 1 ); continue; }
		int arcp = ((p)<<5) + (g->rotation>>1) + 14;
		drawArc( 36, 20, size, arcp, arcp+32, 1 );
		if( size > 65536*19 )
		{
			int applyto = (arcp)/32;
			if( applyto < 0 ) applyto += RADIIS;
			if( applyto >= RADIIS ) applyto -= RADIIS;
			g->alertTime = gameTimeUs;
			if( g->consumed[applyto] < 2 )
			{
				g->score ++;
				g->alertValue = "GOOD";
				g->consumed[applyto]++;
			}
			else
			{
				g->alertValue = "FULL";
			}
			g->panels[i] = -1;
			if( i == sizeof(g->panels)-1 )
			{
				nextSubMode = 2;
			}
		}
	}

	for( i = 0; i < RADIIS; i++ )
	{
		//drawArc( 36, 20, 65536*20, 0, 32*6, 1 );
		int arcp = ((i)<<5) + 14;
		int r = g->consumed[i];
		if( r > 0 )
			drawArc( 36, 20, 65536*20-100, arcp, arcp+32, 1 );
		if( r > 1 )
			drawArc( 36, 20, 65536*20-140100, arcp, arcp+32, 1 );
	}

	if( g->rotdir )
	{
		g->rotation += ((g->rotdir>0)?1:-1) * dstg;
		if( g->rotdir > 0 )
		{
			g->rotdir -= dstg;
			if( g->rotdir < 0 ) g->rotdir = 0;
		}
		if( g->rotdir < 0 )
		{
			g->rotdir += dstg;
			if( g->rotdir > 0 ) g->rotdir = 0;
		}
		if( g->rotation < 0 ) g->rotation += 64*RADIIS;
		if( g->rotation >= 64*RADIIS ) g->rotation -= 64*RADIIS;
	}

	if( buttonMask && g->rotdir == 0 )
	{
		if( buttonMask & 1 ) g->rotdir = -64;
		if( buttonMask & 2 ) g->rotdir = 64;
	}

	if( gameTimeUs - g->alertTime < (100<<12) && g->alertValue )
	{
		swadgeDraw( 0, 2, 0, swadgeGlyphHalf, g->alertValue );
	}

//	drawArc( 36, 20, 20, 0, 50, 1 );
//	drawArc( 36, 20, 18, 0, 94, 1 );

	if( g->subMode > 0 )
	{
		swadgeDraw( 1, SSD1306_H-10, 0, swadgeGlyphHalf, "%d", g->score );
	}

	switch( g->subMode )
	{
	case 0:
		if( st > 250 ) nextSubMode = 1;
		if( st > 10 ) swadgeDraw( SSD1306_W/2, 4, 1, swadgeGlyphHalf, "ROTATE" );
		if( st > 60 ) swadgeDraw( SSD1306_W/2, 15, 1, swadgeGlyphHalf, "YOUR" );
		if( st > 110 ) swadgeDraw( SSD1306_W/2, 25, 1, swadgeGlyphHalf, "OWL" );
		break;
	case 1:
		break;
	case 2:
		glyphdraw_invert = 0;
		glyphdraw_nomask = 0;
		swadgeDraw( SSD1306_W/2, 3, 1, swadgeGlyphHalf, "CAUGHT" );
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyphHalf, "%d", g->score*50 );
		if( st > 400 ) { totalScore += g->score*50; return 1; }
		break;
	}


	if( nextSubMode != g->subMode )
	{
		g->subMode = nextSubMode;
		g->switchTime = gameTimeUs;
	}

	return 0;
}

int GameTrackAndField()
{
	#define RADIIS 10
	struct
	{
		int subMode;
		int switchTime;
		int lastTime;
		int lastStg;

		int rotation;
		int rotspeed;
		int lastbtn;
		int timeleft;
		int score;
	} * g = (void*)gameData;
	int nextSubMode = g->subMode;

	if( gameTimeUs == 0 )
	{
		g->rotation = 0;
	}

	int st = (gameTimeUs - g->switchTime)>>12;
	int dt = st - g->lastTime;
	g->lastTime = st;
	background(((g->subMode == 1) && st < 200 ) ? 7 : 0);

	int drawRotation = 1000-((g->rotation >> 8)%640);
	drawArc( 20, 20, 5*65536, drawRotation,       drawRotation + 64, 1 );
	drawArc( 20, 20, 7*65536, drawRotation + 117, drawRotation + 64 + 117, 1 );
	drawArc( 20, 20, 9*65536, drawRotation + 143, drawRotation + 64 + 143, 1 );
	drawArc( 20, 20, 11*65536, 0, 320, 1 );

	int rg10 = (g->rotation >> 10) - 12;

	switch( g->subMode )
	{
	case 0:
		if( st > 250 ) nextSubMode = 1;
		if( st > 10 ) swadgeDraw( SSD1306_W/2, 4, 1, swadgeGlyphHalf, "TRACK" );
		if( st > 60 ) swadgeDraw( SSD1306_W/2, 15, 1, swadgeGlyphHalf, "AND" );
		if( st > 110 ) swadgeDraw( SSD1306_W/2, 25, 1, swadgeGlyphHalf, "FIELD" );
		break;
	case 1:
	{
		if( buttonEventDown && ( buttonEventDown != g->lastbtn ) )
		{
			g->lastbtn = buttonEventDown;
			g->rotspeed += 125;
		}

		if( rg10 >= 8000 )
		{
			rg10 = 8000;
			nextSubMode = 2;
		}

		g->timeleft =  72-(st>>5);

		if( g->timeleft <= 0 )
		{
			nextSubMode = 2;
		}
	}
	case 2:
	{
		g->rotspeed -= dt;
		if( g->rotspeed < 0 ) g->rotspeed = 0;
		g->rotation += g->rotspeed * dt;

		ssd1306_drawFastHLine( 0, 31, SSD1306_W, 1 );
		for( int i = 0; i < 10; i++ )
		{
			int xofs = (7-(rg10%8)) + i * 8;
			int deep = (((rg10)/8 + i) % 10) == 0;
			ssd1306_drawFastVLine( xofs, 31, 3 + deep * 3, 1 );
		}
		glyphdraw_nomask = 1;

		if( g->subMode == 1 && rg10 > 0 )
		{
			g->score = (rg10/8);
		}
		swadgeDraw( SSD1306_W-1, 4, 2, swadgeGlyphHalf, "%d", g->score );
		ssd1306_drawFastHLine( 0, 0, g->timeleft, 1 );
		break;
	}
	}

	if( g->subMode == 2 )
	{
		swadgeDraw( SSD1306_W/2, 14, 1, swadgeGlyph, (g->score == 1000)?"JOMP":"QWOP" );
		if( st > 350 )
		{
			totalScore += g->score;
			return 1;
		}
	}

	if( nextSubMode != g->subMode )
	{
		g->subMode = nextSubMode;
		g->switchTime = gameTimeUs;
		g->lastTime = 0;
	}

	return 0;
}

int GameMode1up()
{
	background(9+gameModeID);
	int st = gameTimeUs>>11;
	if( st > 400 )
	{
		glyphdraw_invert = 0;
		swadgeDraw( SSD1306_W/2, 3, 1, swadgeGlyphHalf, (const char*[]){"1 UP","FREEEBIE","100 RINGS", "NHL 96"}[gameModeID] );
	}
	if( st > 800 )
	{
		glyphdraw_invert = ( (gameTimeUs>>8) & 1 ) - 1;
		swadgeDraw( SSD1306_W/2, 20, 1, swadgeGlyph, "%d", 1000 );
	}
	if( st > 1800 )
	{
		totalScore += 1000;
		return 1;
	}	
	return 0;
}

int GameModeMainMenu()
{
	struct
	{
		int subMode;
		uint32_t switchTime;
	} * g = (void*)gameData;

	int nextSubMode = g->subMode;
	uint32_t modeDT = gameTimeUs - g->switchTime;
	backgroundAttrib = modeDT>>16;
	background( (const int[]){ 13, 9, 9, 9, 7, 0, 0,0 }[nextSubMode] );

	switch( nextSubMode )
	{
	case 0:
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "BINGBOX" );
		swadgeDraw( SSD1306_W/2, 15, 1, swadgeGlyphHalf, "V1.0" );
		swadgeDraw( 16, 28, 1, swadgeGlyphHalf, "GO" );
		swadgeDraw( 56, 28, 1, swadgeGlyphHalf, "SHO" );
		break;
	case 1:
	case 2:
	{
		uint32_t disp = (nextSubMode == 1)? *highScorePtr : totalScore;
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, (nextSubMode == 1)?"HIGH":"LAST" );

		int pxup = modeDT >> 16;
		int pxupo = pxup;
		if( pxup > 20 ) pxupo = 20;
		swadgeDraw( SSD1306_W/2, pxupo, 1, swadgeGlyph, "%d", disp );
		break;
	}
	case 3:
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "WIPE" );
		swadgeDraw( 16, 28, 1, swadgeGlyphHalf, "YES" );

		if( buttonEventDown )
		{
			nextSubMode =  (buttonEventDown & 2)?0:5;
		}
		break;
	case 5:
	case 6:
	case 7:
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, (const char *[]){"SURE", "REALLY", "4 REAL"}[nextSubMode-5] );

		int oddMode = (nextSubMode & 1);
		swadgeDraw( 16, 28, 1, swadgeGlyphHalf, (oddMode)?"NO":"YES" );
		swadgeDraw( 56, 28, 1, swadgeGlyphHalf, (oddMode)?"YES":"NO" );

		if( buttonEventDown )
		{
			int b1 = buttonEventDown & 1;
			g->switchTime = gameTimeUs;
			nextSubMode =  ((oddMode) ^ (b1)) ? (nextSubMode + 1) : 0;
		}
		break;
	case 8:
		swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyph, "OK..." );
		if( modeDT > 1000000 )
		{
			WriteHighScore( -1 );
			nextSubMode = 0;
		}
		break;
	case 4:
		if( modeDT > 1500000 )
			return 1;
		swadgeDraw( 72 - (modeDT>>12), 8, 0, swadgeGlyph, "LETS GOOOOOOOOOOOOOO" );
		break;
	}

	if( g->subMode < 3 )
	{
		if( (buttonEventDown & 2 ) )
		{
			nextSubMode = (g->subMode+1)%4;
		}

		if( buttonEventDown & 1 )
		{
			nextSubMode = 4;
		}
	}
	

	if( nextSubMode != g->subMode )
	{
		g->switchTime = gameTimeUs;
		g->subMode = nextSubMode;
	}
	return 0;
}

int GameModeEnding()
{
	background(8);
	swadgeDraw( SSD1306_W/2, 2, 1, swadgeGlyphHalf, "FINAL" );

	if( gameTimeUs == 0 )
	{
		// TODO: Check high score.
		//printf( "COMP: %d\n", totalScore > *highScorePtr );
		if( totalScore > *highScorePtr )
		{
			WriteHighScore( totalScore );
		}
	}

	int pxup = gameTimeUs >> 16;
	int pxupo = pxup;
	if( pxup > 20 ) pxupo = 20;
	swadgeDraw( SSD1306_W/2, pxupo, 1, swadgeGlyph, "%d", totalScore );

	if( pxup > 64 )
	{
		if( buttonEventDown )
		{
			return 1;
		}
	}

	return 0;
}

int GameModeInterstitial()
{

	glyphdraw_invert = 0;
	glyphdraw_nomask = 1;

	backgroundAttrib = gameTimeUs>>13;
	background(13);
	swadgeDraw( 0, 2, 0, swadgeGlyphHalf, "GAME" );
	
	swadgeDraw( SSD1306_W, 2, 2, swadgeGlyphHalf, ( gameNumber == maxGames )?"LAST":"%d/%d", gameNumber, maxGames );

	swadgeDraw( SSD1306_W/2, 16, 1, swadgeGlyph, "%d", totalScore );

	if( gameTimeUs>>12 > 400 || buttonEventDown )
	{
		return 1;
	}
	

	return 0;
}



gamemode_t gameModeForce = 0;
int        gameModeForceID = 0;


gamemode_t gameModes[8] = {
	GameModeGrand, GameModeTunnel, GameModeTunnel, GameModeFlap, GameModeSnek, GameBeatTop, RadialGame, GameTrackAndField
};

int gameModeIDs[8] = {
	0, 0, 1, 0, 0, 0, 0, 0,
};

uint8_t gamePlayedCount[8];

gamemode_t gameModesFree[] = {
	GameMode1up, GameMode1up, GameMode1up, GameMode1up,
};

int gameModeFreeIDs[] = {
	0, 1, 2, 3,
};


#endif

