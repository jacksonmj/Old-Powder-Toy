#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <SDL/SDL.h>

#undef PLOSS
#define FLAG_STAGNANT 1
#define XRES	480
#define YRES	232
#define CELL    4
#define ISTP    (CELL/2)
#define CFDS 	(4.0f/CELL)
char *it_msg =
    "\brThe Powder Toy\n"
    "\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\x7F\n"
    "\bwMaterials on the bottom right are walls, materials on the bottom left are particles.\n"
    "\n"
    "Pick your material from the bottom bar using the square and circle buttons.\n"
    "Draw freeform lines by using the right trigger and joystick.\n"
    "The left trigger draws straight lines, and the triangle button draws boxes.\n"
    "\n"
    "\bg(c) 2008 Stanislaw K Skowronek   \bbirc.unaligned.org #wtf\n"
    ;

/***********************************************************
 *                   AIR FLOW SIMULATOR                    *
 ***********************************************************/

unsigned char bmap[YRES/CELL][XRES/CELL];

float vx[YRES/CELL][XRES/CELL], ovx[YRES/CELL][XRES/CELL];
float vy[YRES/CELL][XRES/CELL], ovy[YRES/CELL][XRES/CELL];
float pv[YRES/CELL][XRES/CELL], opv[YRES/CELL][XRES/CELL];
#define TSTEPP 0.3f
#define TSTEPV 0.4f
#define VADV 0.3f
#define VLOSS 0.995f
#define PLOSS 0.995f

float kernel[9];
void make_kernel(void)
{
    int i, j;
    float s = 0.0f;
    for(j=-1; j<2; j++)
	for(i=-1; i<2; i++) {
	    kernel[(i+1)+3*(j+1)] = exp(-5.0f*(i*i+j*j));
	    s += kernel[(i+1)+3*(j+1)];
	}
    s = 1.0f / s;
    for(j=-1; j<2; j++)
	for(i=-1; i<2; i++)
	    kernel[(i+1)+3*(j+1)] *= s;
}

void update_air(void)
{
    int x, y, i, j;
    float dp, dx, dy, f, tx, ty;

    for(y=1; y<YRES/CELL; y++) {
	for(x=1; x<XRES/CELL; x++) {
	    dp = 0.0f;
	    dp += vx[y][x-1] - vx[y][x];
	    dp += vy[y-1][x] - vy[y][x];
	    pv[y][x] *= PLOSS;
	    pv[y][x] += dp*TSTEPP;
	}
    }
    for(y=0; y<YRES/CELL-1; y++) {
	for(x=0; x<XRES/CELL-1; x++) {
	    dx = dy = 0.0f;
	    dx += pv[y][x] - pv[y][x+1];
	    dy += pv[y][x] - pv[y+1][x];
	    vx[y][x] *= VLOSS;
	    vy[y][x] *= VLOSS;
	    vx[y][x] += dx*TSTEPV;
	    vy[y][x] += dy*TSTEPV;
	    if(bmap[y][x]==1 || bmap[y][x+1]==1)
		vx[y][x] = 0;
	    if(bmap[y][x]==1 || bmap[y+1][x]==1)
		vy[y][x] = 0;
	}
}
	

    for(y=0; y<YRES/CELL; y++)
	for(x=0; x<XRES/CELL; x++) {
	    dx = 0.0f;
	    dy = 0.0f;
	    dp = 0.0f;
	    for(j=-1; j<2; j++)
		for(i=-1; i<2; i++)
		    if(y+j>0 && y+j<YRES/CELL-1 &&
		       x+i>0 && x+i<XRES/CELL-1 &&
		       bmap[y+j][x+i]!=1) {
			f = kernel[i+1+(j+1)*3];
		        dx += vx[y+j][x+i]*f;
		        dy += vy[y+j][x+i]*f;
			dp += pv[y+j][x+i]*f;
		    } else {
			f = kernel[i+1+(j+1)*3];
		        dx += vx[y][x]*f;
		        dy += vy[y][x]*f;
			dp += pv[y][x]*f;
		    }

	    tx = x - dx;
	    ty = y - dy;
	    i = (int)tx;
	    j = (int)ty;
	    tx -= i;
	    ty -= j;
	    if(i>=0 && i<XRES/CELL-2 &&
	       j>=0 && j<YRES/CELL-2) {
		dx *= 1.0f - VADV;
		dy *= 1.0f - VADV;

		dx += VADV*(1.0f-tx)*(1.0f-ty)*vx[j][i];
		dy += VADV*(1.0f-tx)*(1.0f-ty)*vy[j][i];

		dx += VADV*tx*(1.0f-ty)*vx[j][i+1];
		dy += VADV*tx*(1.0f-ty)*vy[j][i+1];

		dx += VADV*(1.0f-tx)*ty*vx[j+1][i];
		dy += VADV*(1.0f-tx)*ty*vy[j+1][i];

		dx += VADV*tx*ty*vx[j+1][i+1];
		dy += VADV*tx*ty*vy[j+1][i+1];
	    }

	    if(dp > 256.0f) dp = 256.0f;
	    if(dp < -256.0f) dp = -256.0f;
	    if(dx > 256.0f) dx = 256.0f;
	    if(dx < -256.0f) dx = -256.0f;
	    if(dy > 256.0f) dy = 256.0f;
	    if(dy < -256.0f) dy = -256.0f;

	    ovx[y][x] = dx;
	    ovy[y][x] = dy;
	    opv[y][x] = dp;
	}
    memcpy(vx, ovx, sizeof(vx));
    memcpy(vy, ovy, sizeof(vy));
    memcpy(pv, opv, sizeof(pv));
}

unsigned clamp_flt(float f, float min, float max)
{
    if(f<min)
	return 0;
    if(f>max)
	return 255;
    return (int)(255.0f*(f-min)/(max-min));
}

void draw_air(unsigned *vid)
{
    int x, y, i, j;
    unsigned c;

    for(y=0; y<YRES/CELL; y++)
	for(x=0; x<XRES/CELL; x++) {
	  #ifdef BGR
            c  = clamp_flt(pv[y][x], 0.0f, 8.0f) << 8;
            c |= clamp_flt(fabs(vx[y][x]), 0.0f, 8.0f);
            c |= clamp_flt(fabs(vy[y][x]), 0.0f, 8.0f) << 16;
            #else
            c  = clamp_flt(pv[y][x], 0.0f, 8.0f) << 8;
            c |= clamp_flt(fabs(vx[y][x]), 0.0f, 8.0f) << 16;
            c |= clamp_flt(fabs(vy[y][x]), 0.0f, 8.0f);
            #endif
	    for(j=0; j<CELL; j++)
		for(i=0; i<CELL; i++)
		    vid[(x*CELL+i) + (y*CELL+j)*XRES] = c;
	}
}

/***********************************************************
 *                   PARTICLE SIMULATOR                    *
 ***********************************************************/

#define NPART 16384

#define PT_NONE	0
#define PT_DUST	1
#define PT_WATR	2
#define PT_OILL 3
#define PT_FIRE 4
#define PT_METL 5
#define PT_LAVA 6
#define PT_GUNP	7
#define PT_NITR	8
#define PT_CLNE 9
#define PT_GASS 10
#define PT_PLEX 11
#define PT_DFRM 12
#define PT_ICEI 13
#define PT_WIRE 14
#define PT_SPRK 15
#define PT_SNOW 16
#define PT_WOOD 17
#define PT_NEUT 18
#define PT_PLUT 19
#define PT_NUM  20

char *names[] = {
    "",
    "DUST",
    "WATR",
    "OIL",
    "FIRE",
    "STNE",
    "LAVA",
    "GUN",
    "NITR",
    "CLNE",
    "GAS",
    "C-4",
    "GOO",
    "ICE",
    "METL",
    "SPRK",
    "SNOW",
    "WOOD",
    "NEUT",
    "PLUT",
};
char *descs[] = {
    "Erases particles.",
    "Very light dust. Flammable.",
    "Liquid. Freezes. Extinguishes fires.",
    "Liquid. Flammable.",
    "Ignites flammable materials. Heats air.",
    "Heavy particles. Meltable.",
    "Heavy liquid. Ignites flammable materials. Solidifies when cold.",
    "Light dust. Explosive.",
    "Liquid. Pressure sensitive explosive.",
    "Solid. Duplicates any particles it touches.",
    "Gas. Diffuses. Flammable. Liquifies under pressure.",
    "Solid. Pressure sensitive explosive.",
    "Solid. Deforms and disappears under pressure.",
    "Solid. Freezes water. Crushes under pressure. Cools down air.",
    "Solid. Conducts electricity. Meltable.",
    "Electricity. Conducted by metal.",
    "Light particles.",
    "Solid. Flammable.",
    "Neutrons. Interact with matter in odd ways.",
    "Heavy particles. Fissile. Generates neutrons under pressure.",
};
#ifdef BGR
unsigned pcolors[] = { // BGR
    0x000000,
    0xA0E0FF,
    0xD03020,
    0x104040,
    0x0010FF,
    0xA0A0A0,
    0x1050E0,
    0xD0C0C0,
    0x10E020,
    0x10D0FF,
    0x20FFE0,
    0xE080D0,
    0x004080,
    0xFFC0A0,
    0x604040,
    0x80FFFF,
    0xFFE0C0,
    0x40A0C0,
    0xFFE020,
    0x207040,
};
#else
unsigned pcolors[] = {
    0x000000,
    0xFFE0A0,
    0x2030D0,
    0x404010,
    0xFF1000,
    0xA0A0A0,
    0xE05010,
    0xC0C0D0,
    0x20E010,
    0xFFD010,
    0xE0FF20,
    0xD080E0,
    0x804000,
    0xA0C0FF,
    0x404060,
    0xFFFF80,
    0xC0E0FF,
    0xC0A040,
    0x20E0FF,
    0x407020,
};
#endif
float advection[] = {
    0.0f,
    0.7f,
    0.6f,
    0.6f,
    0.9f,
    0.4f,
    0.3f,
    0.7f,
    0.5f,
    0.0f,
    1.0f,
    0.0f,
    0.1f,
    0.0f,
    0.0f,
    0.0f,
    0.7f,
    0.0f,
    0.0f,
    0.4f,
};
float airdrag[] = {
    0.00f * CFDS,
     0.03f * CFDS,
     0.01f * CFDS,
     0.01f * CFDS,
     0.04f * CFDS,
     0.04f * CFDS,
     0.02f * CFDS,
     0.03f * CFDS,
     0.02f * CFDS,
     0.00f * CFDS,
     0.01f * CFDS,
     0.00f * CFDS,
     0.00f * CFDS,
     0.00f * CFDS,
     0.00f * CFDS,
     0.00f * CFDS,
     0.01f * CFDS,
     0.00f * CFDS,
     0.00f * CFDS,
     0.02f * CFDS,
};
float airloss[] = {
    1.00f,
    0.96f,
    0.98f,
    0.98f,
    0.97f,
    0.94f,
    0.95f,
    0.94f,
    0.92f,
    0.90f,
    0.99f,
    0.90f,
    0.97f,
    0.90f,
    0.90f,
    0.90f,
    0.96f,
    0.90f,
    1.00f,
    0.98f,
};
float loss[] = {
    0.00f,
    0.80f,
    0.95f,
    0.95f,
    0.20f,
    0.95f,
    0.80f,
    0.80f,
    0.97f,
    0.00f,
    0.30f,
    0.00f,
    0.50f,
    0.00f,
    0.00f,
    0.00f,
    0.90f,
    0.00f,
    1.00f,
    0.95f,
};
float collision[] = {
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -0.1f,
    0.0f,
    0.0f,
    -0.1f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    -0.1f,
    0.0f,
    -0.99f,
    0.0f,
};
float gravity[] = {
    0.0f,
    0.1f,
    0.1f,
    0.1f,
    -0.05f,
    0.3f,
    0.15f,
    0.1f,
    0.2f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.05f,
    0.0f,
    0.0f,
    0.4f,
};
float diffusion[] = {
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.75f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.00f,
    0.01f,
    0.00f,
    0.01f,
    0.00f,
};
float hotair[] = {
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.005f   * CFDS,
     0.000f   * CFDS,
     0.0003f  * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.001f   * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     -0.0003f * CFDS,
     0.000f   * CFDS,
     0.001f   * CFDS,
     0.000f   * CFDS,
     0.000f   * CFDS,
     0.002f   * CFDS,
     0.000f   * CFDS,
};
int falldown[] = {
    0,
    1,
    2,
    2,
    1,
    1,
    2,
    1,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    0,
    0,
    1,
};
int flammable[] = {
    0,
    30,
    0,
    100,
    0,
    2,
    0,
    600,
    1000,
    0,
    600,
    1000,
    0,
    0,
    0,
    0,
    0,
    30,
    0,
    0,
};
int explosive[] = {
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    1,
    2,
    0,
    0,
    2,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
};
int meltable[] = {
    0,
    0,
    0,
    0,
    0,
    5,
    0,
    0,
    0,
    0,
    0,
    50,
    0,
    0,
    1,
    0,
    0,
    0,
    0,
    0,
};

typedef struct {
    int type;
    int life, ctype;
    float x, y, vx, vy;
    int flags;
} particle;
particle *parts;

int pfree;

unsigned pmap[YRES][XRES];

int try_move(int i, int x, int y, int nx, int ny)
{
    unsigned r;

    if(nx<0 || ny<0 || nx>=XRES || ny>=YRES)
	return 0;
    if(x==nx && y==ny)
	return 1;
    r = pmap[ny][nx];
    if(r && (r>>8)<NPART)
	r = (r&~0xFF) | parts[r>>8].type;

    if(falldown[parts[i].type]!=2 && bmap[ny/CELL][nx/CELL]==3)
	return 0;
    if(r && (r>>8)<NPART && falldown[r&0xFF]!=2 && bmap[y/CELL][x/CELL]==3)
	return 0;

    if(r &&
       !(parts[i].type==PT_METL && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_DUST || (r&0xFF)==PT_SNOW || (r&0xFF)==PT_GASS || (r&0xFF)==PT_GUNP || (r&0xFF)==PT_NITR || (r&0xFF)==PT_LAVA)) &&
       !(parts[i].type==PT_PLUT && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_DUST || (r&0xFF)==PT_SNOW || (r&0xFF)==PT_GASS || (r&0xFF)==PT_GUNP || (r&0xFF)==PT_NITR || (r&0xFF)==PT_LAVA)) &&
       !(parts[i].type==PT_LAVA && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_DUST || (r&0xFF)==PT_SNOW || (r&0xFF)==PT_GASS || (r&0xFF)==PT_GUNP || (r&0xFF)==PT_NITR)) &&
       !(parts[i].type==PT_DUST && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_GASS || (r&0xFF)==PT_NITR)) &&
       !(parts[i].type==PT_SNOW && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_GASS || (r&0xFF)==PT_NITR)) &&
       !(parts[i].type==PT_GUNP && ((r&0xFF)==PT_WATR || (r&0xFF)==PT_OILL || (r&0xFF)==PT_GASS || (r&0xFF)==PT_NITR)) &&
       !(parts[i].type==PT_WATR && ((r&0xFF)==PT_OILL || (r&0xFF)==PT_NITR || (r&0xFF)==PT_GASS)) &&
       !(parts[i].type==PT_NITR && ((r&0xFF)==PT_OILL || (r&0xFF)==PT_GASS)) &&
       !(parts[i].type==PT_OILL && ((r&0xFF)==PT_GASS || (r&0xFF)==PT_FIRE)) &&
       !(parts[i].type==PT_LAVA && (r&0xFF)==PT_FIRE) &&
       !(parts[i].type==PT_FIRE && (r&0xFF)==PT_LAVA) &&
       !(parts[i].type==PT_NEUT && (r&0xFF)!=PT_WIRE && (r&0xFF)!=PT_CLNE && (r&0xFF)!=PT_LAVA && (r&0xFF)!=PT_METL))
	return 0;

    pmap[ny][nx] = (i<<8)|parts[i].type;
    pmap[y][x] = r;
    if(r && (r>>8)<NPART) {
	r >>= 8;
	parts[r].x += x-nx;
	parts[r].y += y-ny;
    }

    return 1;
}

void kill_part(int i)
{
    int x, y;
    parts[i].type = PT_NONE;

    x = (int)(parts[i].x+0.5f);
    y = (int)(parts[i].y+0.5f);

    if(x>=0 && y>=0 && x<XRES && y<YRES)
	pmap[y][x] = PT_NONE;

    parts[i].life = pfree;
    pfree = i;
}

int create_part(int p, int x, int y, int t)
{
    int i;

    if(x<0 || y<0 || x>=XRES || y>=YRES)
	return -1;

    if(t==PT_SPRK) {
	if((pmap[y][x]&0xFF) != PT_WIRE)
	    return -1;
	parts[pmap[y][x]>>8].type = PT_SPRK;
	parts[pmap[y][x]>>8].life = 4;
	pmap[y][x] = (pmap[y][x]&~0xFF) | PT_SPRK;
	return pmap[y][x]>>8;
    }

    if(p==-1) {
	if(pmap[y][x])
	    return -1;
	if(pfree == -1)
	    return -1;
	i = pfree;
	pfree = parts[i].life;
    } else
	i = p;

    parts[i].x = x;
    parts[i].y = y;
    parts[i].type = t;
    parts[i].vx = 0;
    parts[i].vy = 0;
    parts[i].life = 0;
    parts[i].ctype = 0;
    if(t==PT_FIRE)
	parts[i].life = rand()%50+120;
    if(t==PT_LAVA)
	parts[i].life = rand()%120+240;
    if(t==PT_NEUT) {
	float r = (rand()%128+128)/127.0f;
	float a = (rand()%360)*3.14159f/180.0f;
	parts[i].life = rand()%120+480;
        parts[i].vx = r*cos(a);
        parts[i].vy = r*sin(a);
    }

    pmap[y][x] = t|(i<<8);

    return i;
}

void delete_part(int x, int y)
{
    unsigned i;

    if(x<0 || y<0 || x>=XRES || y>=YRES)
	return;
    i = pmap[y][x];
    if(!i || (i>>8)>=NPART)
	return;

    kill_part(i>>8);
}
#ifdef BGR
void blendpixel(unsigned *vid, int x, int y, int b, int g, int r, int a)
#else
void blendpixel(unsigned *vid, int x, int y, int r, int g, int b, int a)
#endif
{
    int t;
    if(x<0 || y<0 || x>=XRES || y>=YRES)
	return;
    if(a!=255) {
	t = vid[y*XRES+x];
	r = (a*r + (255-a)*((t>>16)&255)) >> 8;
	g = (a*g + (255-a)*((t>>8)&255)) >> 8;
	b = (a*b + (255-a)*(t&255)) >> 8;
    }
    vid[y*XRES+x] = (r<<16)|(g<<8)|b;
}

void update_particles(unsigned *vid)
{
    int i, j, x, y, t, nx, ny, r, a, cr,cg,cb, s, l = -1, rt, fe, nt;
    float mv, dx, dy, ix, iy, lx, ly;

    memset(pmap, 0, sizeof(pmap));
    r = rand()%2;
    for(j=0; j<NPART; j++) {
	i = r ? (NPART-1-j) : j;
	if(parts[i].type) {
	    t = parts[i].type;
	    x = (int)(parts[i].x+0.5f);
	    y = (int)(parts[i].y+0.5f);
	    if(x>=0 && y>=0 && x<XRES && y<YRES)
		pmap[y][x] = t|(i<<8);
	} else {
	    parts[i].life = l;
	    l = i;
	}
    }
    pfree=l;

    for(y=0; y<YRES/CELL; y++)
	for(x=0; x<XRES/CELL; x++) {
	    if(bmap[y][x]==1)
		for(j=0; j<CELL; j++)
		    for(i=0; i<CELL; i++) {
			pmap[y*CELL+j][x*CELL+i] = 0x7FFFFFFF;
			vid[(y*CELL+j)*XRES+(x*CELL+i)] = 0x808080;
		    }
	    if(bmap[y][x]==2)
		for(j=0; j<CELL; j+=2)
		    for(i=(j>>1)&1; i<CELL; i+=2)
			vid[(y*CELL+j)*XRES+(x*CELL+i)] = 0x808080;
	    if(bmap[y][x]==3)
		for(j=0; j<CELL; j++)
		    for(i=0; i<CELL; i++)
			if(!((y*CELL+j)%2) && !((x*CELL+i)%2))
			    vid[(y*CELL+j)*XRES+(x*CELL+i)] = 0x808080;
	}

    for(i=0; i<NPART; i++)
	if(parts[i].type) {
	    t = parts[i].type;

	    if(parts[i].life) {
		parts[i].life--;
		if(parts[i].life<=0 && t!=PT_WIRE && t!=PT_SPRK && t!=PT_LAVA) {
		    kill_part(i);
		    continue;
		}
		if(parts[i].life<=0 && t==PT_SPRK) {
		    t = parts[i].type = PT_WIRE;
		    parts[i].life = 4;
		}
	    }

	    x = (int)(parts[i].x+0.5f);
	    y = (int)(parts[i].y+0.5f);

	    if(x<0 || y<0 || x>=XRES || y>=YRES ||
	       bmap[y/CELL][x/CELL]==1 || bmap[y/CELL][x/CELL]==2 ||
	       (bmap[y/CELL][x/CELL]==3 && falldown[t]!=2)) {
		kill_part(i);
		continue;
	    }

	    vx[y/CELL][x/CELL] *= airloss[t];
	    vy[y/CELL][x/CELL] *= airloss[t];
	    vx[y/CELL][x/CELL] += airdrag[t]*parts[i].vx;
	    vy[y/CELL][x/CELL] += airdrag[t]*parts[i].vy;
	    pv[y/CELL][x/CELL] += hotair[t];
	    if(y+CELL<YRES)
		pv[y/CELL+1][x/CELL] += hotair[t];
	    if(x+CELL<YRES) {
		pv[y/CELL][x/CELL+1] += hotair[t];
		if(y+CELL<YRES)
		    pv[y/CELL+1][x/CELL+1] += hotair[t];
	    }

	    if((explosive[t]&2) && pv[y/CELL][x/CELL]>1.5f) {
		parts[i].life = rand()%80+180;
		parts[i].type = PT_FIRE;
		pv[y/CELL][x/CELL] += 0.25f * CFDS;
		t = PT_FIRE;
	    }

	    parts[i].vx *= loss[t];
	    parts[i].vy *= loss[t];

	    if(t==PT_DFRM && !parts[i].life) {
		if(pv[y/CELL][x/CELL]>1.0f) {
		    parts[i].vx += advection[t]*vx[y/CELL][x/CELL];
		    parts[i].vy += advection[t]*vy[y/CELL][x/CELL];
		    parts[i].life = rand()%80+300;
		}
	    } else {
		parts[i].vx += advection[t]*vx[y/CELL][x/CELL];
		parts[i].vy += advection[t]*vy[y/CELL][x/CELL] + gravity[t];
	    }

	    if(diffusion[t]) {
		parts[i].vx += diffusion[t]*(rand()/(0.5f*RAND_MAX)-1.0f);
		parts[i].vy += diffusion[t]*(rand()/(0.5f*RAND_MAX)-1.0f);
	    }

	    // interpolator
	    mv = fmaxf(fabs(parts[i].vx), fabs(parts[i].vy));
	    lx = parts[i].x;
	    ly = parts[i].y;
	    if(mv < ISTP) {
		parts[i].x += parts[i].vx;
		parts[i].y += parts[i].vy;
		ix = parts[i].x;
		iy = parts[i].y;
	    } else {
		dx = parts[i].vx*ISTP/mv;
		dy = parts[i].vy*ISTP/mv;
		ix = parts[i].x;
		iy = parts[i].y;
		while(1) {
		    mv -= ISTP;
		    if(mv <= 0.0f) {
			// nothing found
			parts[i].x += parts[i].vx;
			parts[i].y += parts[i].vy;
			ix = parts[i].x;
			iy = parts[i].y;
			break;
		    }
		    ix += dx;
		    iy += dy;
		    nx = (int)(ix+0.5f);
		    ny = (int)(iy+0.5f);
		    if(nx<0 || ny<0 || nx>=XRES || ny>=YRES || pmap[ny][nx] || bmap[ny/CELL][nx/CELL]) {
			parts[i].x = ix;
			parts[i].y = iy;
			break;
		    }
		}
	    }

	    a = nt = 0;
	    for(nx=-1; nx<2; nx++)
		for(ny=-1; ny<2; ny++)
		    if(x+nx>=0 && y+ny>0 &&
		       x+nx<XRES && y+ny<YRES &&
     			!bmap[(y+ny)/CELL][(x+nx)/CELL]) {
                       if(!pmap[y+ny][x+nx])
                           a = 1;
                       if((pmap[y+ny][x+nx]&0xFF)!=t)
                           nt = 1;                   }

	    if(t==PT_GASS && pv[y/CELL][x/CELL]>4.0f)
		t = parts[i].type = PT_OILL;
	    if(t==PT_ICEI && pv[y/CELL][x/CELL]>0.3f)
		t = parts[i].type = PT_SNOW;
	    if(t==PT_PLUT && 1>rand()%100 && (1+(int)(5.0f*pv[y/CELL][x/CELL]))>(rand()%1000)) {
		create_part(i, x, y, PT_NEUT);
		t = PT_NEUT;
	    }

	    if(t==PT_ICEI || t==PT_SNOW) {
		for(nx=-2; nx<3; nx++)
		    for(ny=-2; ny<3; ny++)
			if(x+nx>=0 && y+ny>0 &&
			   x+nx<XRES && y+ny<YRES && (nx || ny)) {
			    r = pmap[y+ny][x+nx];
			    if((r>>8)>=NPART || !r)
				continue;
			    if((r&0xFF)==PT_WATR && 1>(rand()%1000)) {
				t = parts[i].type = PT_ICEI;
				parts[r>>8].type = PT_ICEI;
			    }
			    if(t==PT_SNOW && (r&0xFF)==PT_WATR && 15>(rand()%1000))
				t = parts[i].type = PT_WATR;
			}
	    }

	    fe = 0;
	    if(t==PT_NEUT) {
		rt = 1 + (int)pv[y/CELL][x/CELL];
		for(nx=-1; nx<2; nx++)
		    for(ny=-1; ny<2; ny++)
			if(x+nx>=0 && y+ny>0 &&
			   x+nx<XRES && y+ny<YRES && (nx || ny)) {
			    r = pmap[y+ny][x+nx];
			    if((r>>8)>=NPART || !r)
				continue;
			    if((r&0xFF)==PT_PLUT && rt>(rand()%1000)) {
				create_part(r>>8, x+nx, y+ny, PT_NEUT);
				parts[r>>8].vx = 0.25f*parts[r>>8].vx + parts[i].vx;
				parts[r>>8].vy = 0.25f*parts[r>>8].vy + parts[i].vy;
				pv[y/CELL][x/CELL] += 2.00f * CFDS;
				fe ++;
			    }                           
			if((r&0xFF)==PT_GUNP && 15>(rand()%1000))
                               parts[r>>8].type = PT_DUST;
                           if((r&0xFF)==PT_PLEX && 15>(rand()%1000))
                               parts[r>>8].type = PT_DFRM;
                           if((r&0xFF)==PT_NITR && 15>(rand()%1000))
                               parts[r>>8].type = PT_OILL;
                           if((r&0xFF)==PT_OILL && 5>(rand()%1000))
                               parts[r>>8].type = PT_GASS;

			}
	    }

	    if(t==PT_FIRE || t==PT_LAVA || t==PT_SPRK || fe) {
		for(nx=-2; nx<3; nx++)
		    for(ny=-2; ny<3; ny++)
			if(x+nx>=0 && y+ny>0 &&
			   x+nx<XRES && y+ny<YRES && (nx || ny)) {
			    r = pmap[y+ny][x+nx];
			    if((r>>8)>=NPART || !r)
				continue;
			    if(bmap[(y+ny)/CELL][(x+nx)/CELL])
				continue;
			    rt = parts[r>>8].type;
			    if((a || explosive[rt]) &&
			       (t!=PT_LAVA || parts[i].life>0 || (rt!=PT_METL && rt!=PT_WIRE)) &&
			       flammable[rt] && flammable[rt]>(rand()%1000)) {
				parts[r>>8].type = PT_FIRE;
				parts[r>>8].life = rand()%80+180;
				if(explosive[rt])
				    pv[y/CELL][x/CELL] += 0.25f * CFDS;
				continue;
			    }
			    if(t!=PT_SPRK && meltable[rt] &&
			       meltable[rt]>(rand()%1000)) {
			        if(t!=PT_LAVA || parts[i].life>0) {
				    parts[r>>8].type = PT_LAVA;
				    parts[r>>8].life = rand()%120+240;
				} else {
				    parts[i].life = 0;
				    t = parts[i].type = rt;
				    goto killed;
				}
			    }
			    if(t!=PT_SPRK && (rt==PT_ICEI || rt==PT_SNOW)) {
				parts[r>>8].type = PT_WATR;
				if(t==PT_FIRE) {
				    kill_part(i);
				    goto killed;
				}
				if(t==PT_LAVA) {
				    parts[i].life = 0;
				    t = parts[i].type = PT_METL;
				    goto killed;
				}
			    }
			    if(t!=PT_SPRK && rt==PT_WATR) {
				kill_part(r>>8);
				if(t==PT_FIRE) {
				    kill_part(i);
				    goto killed;
				}
				if(t==PT_LAVA) {
				    parts[i].life = 0;
				    t = parts[i].type = PT_METL;
				    goto killed;
				}
			    }
			    if(t==PT_SPRK && rt==PT_WIRE && parts[r>>8].life==0) {
				parts[r>>8].type = PT_SPRK;
				parts[r>>8].life = 4;
			    }
			}
	    killed:
		if(parts[i].type == PT_NONE)
		    continue;
	    }

	    if(t==PT_CLNE) {
		if(!parts[i].ctype) {
		    for(nx=-1; nx<2; nx++)
			for(ny=-1; ny<2; ny++)
			    if(x+nx>=0 && y+ny>0 &&
		               x+nx<XRES && y+ny<YRES &&
			       pmap[y+ny][x+nx] &&
			       (pmap[y+ny][x+nx]&0xFF)!=PT_CLNE &&
			       (pmap[y+ny][x+nx]&0xFF)!=0xFF)
				parts[i].ctype = pmap[y+ny][x+nx]&0xFF;
		} else
		    create_part(-1, x+rand()%3-1, y+rand()%3-1, parts[i].ctype);
	    }

	    nx = (int)(parts[i].x+0.5f);
	    ny = (int)(parts[i].y+0.5f);

	    if(nx<CELL || nx>=XRES-CELL ||
	       ny<CELL || ny>=YRES-CELL) {
	        kill_part(i);
		continue;
	    }
	  rt = parts[i].flags & FLAG_STAGNANT;
          parts[i].flags &= ~FLAG_STAGNANT;

	    if(!try_move(i, x, y, nx, ny)) {
		parts[i].x = lx;
		parts[i].y = ly;
		if(falldown[t]) {
		    if(nx!=x && try_move(i, x, y, nx, y)) {
			parts[i].x = ix;
			parts[i].vx *= collision[t];
			parts[i].vy *= collision[t];
		    } else if(ny!=y && try_move(i, x, y, x, ny)) {
			parts[i].y = iy;
			parts[i].vx *= collision[t];
			parts[i].vy *= collision[t];
		    } else {
			r = (rand()%2)*2-1;
			if(ny!=y && try_move(i, x, y, x+r, ny)) {
			    parts[i].x += r;
			    parts[i].y = iy;
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
			} else if(ny!=y && try_move(i, x, y, x-r, ny)) {
			    parts[i].x -= r;
			    parts[i].y = iy;
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
			} else if(nx!=x && try_move(i, x, y, nx, y+r)) {
			    parts[i].x = ix;
			    parts[i].y += r;
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
			} else if(nx!=x && try_move(i, x, y, nx, y-r)) {
			    parts[i].x = ix;
			    parts[i].y -= r;
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
			} else if(falldown[t]>1 && parts[i].vy>fabs(parts[i].vx)) {
			    s = 0;
			   if(!rt || nt)
                              rt = 50;
                           else
                              rt = 10;
                           for(j=x+r; j>=0 && j>=x-rt && j<x+rt && j<XRES; j+=r) {
				if(try_move(i, x, y, j, ny)) {
				    parts[i].x += j-x;
				    parts[i].y += ny-y;
				    x = j;
				    y = ny;
				    s = 1;
				    break;
				}
				if(try_move(i, x, y, j, y)) {
				    parts[i].x += j-x;
				    x = j;
				    s = 1;
				    break;
				}
				if((pmap[y][j]&255)!=t || bmap[y/CELL][j/CELL])
				    break;
			    }
			    if(parts[i].vy>0)
				r = 1;
			    else
				r = -1;
			    if(s)
				for(j=y+r; j>=0 && j<YRES && j>=y-rt && j<x+rt; j+=r) {
				    if(try_move(i, x, y, x, j)) {
					parts[i].y += j-y;
					break;
				    }
				    if((pmap[j][x]&255)!=t || bmap[j/CELL][x/CELL]) { 
					s = 0;
					break;
					}
				}
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
    			if(!s)
                            parts[i].flags |= FLAG_STAGNANT;
                        } else {
                            parts[i].flags |= FLAG_STAGNANT;
			    parts[i].vx *= collision[t];
			    parts[i].vy *= collision[t];
			}
		    }
		} else {
		    parts[i].flags |= FLAG_STAGNANT;
                    if(100>(rand()%1000)) {
                        kill_part(i);
                        continue;
                    } else if(try_move(i, x, y, 2*x-nx, ny)) {
			
			parts[i].x = 2*x-nx;
			parts[i].y = iy;
			parts[i].vx *= collision[t];
		    } else if(try_move(i, x, y, nx, 2*y-ny)) {
			parts[i].x = ix;
			parts[i].y = 2*y-ny;
			parts[i].vy *= collision[t];
		    } else {
			parts[i].vx *= collision[t];
			parts[i].vy *= collision[t];
		    }
		}
	    }

	    nx = (int)(parts[i].x+0.5f);
	    ny = (int)(parts[i].y+0.5f);

	    if(nx<CELL || nx>=XRES-CELL ||
	       ny<CELL || ny>=YRES-CELL) {
	        kill_part(i);
		continue;
	    }

	    if(t==PT_NEUT) {
		#ifdef BGR
		cr = pcolors[t]&0xFF;
		cg = (pcolors[t]>>8)&0xFF;
		cb = pcolors[t]>>16;
		#else
		cr = pcolors[t]>>16;
		cg = (pcolors[t]>>8)&0xFF;
		cb = pcolors[t]&0xFF;
		#endif
		blendpixel(vid, nx, ny, cr, cg, cb, 192);
		blendpixel(vid, nx+1, ny, cr, cg, cb, 96);
		blendpixel(vid, nx-1, ny, cr, cg, cb, 96);
		blendpixel(vid, nx, ny+1, cr, cg, cb, 96);
		blendpixel(vid, nx, ny-1, cr, cg, cb, 96);
		blendpixel(vid, nx+1, ny-1, cr, cg, cb, 32);
		blendpixel(vid, nx-1, ny+1, cr, cg, cb, 32);
		blendpixel(vid, nx+1, ny+1, cr, cg, cb, 32);
		blendpixel(vid, nx-1, ny-1, cr, cg, cb, 32);
	    } else if(t==PT_FIRE && parts[i].life) {
		cr = parts[i].life * 8;
		cg = parts[i].life * 2;
		cb = parts[i].life;
		if(cr>255) cr = 255;
		if(cg>192) cg = 212;
		if(cb>128) cb = 192;
		blendpixel(vid, nx, ny, cr, cg, cb, 255);
		blendpixel(vid, nx+1, ny, cr, cg, cb, 64);
		blendpixel(vid, nx-1, ny, cr, cg, cb, 64);
		blendpixel(vid, nx, ny+1, cr, cg, cb, 64);
		blendpixel(vid, nx, ny-1, cr, cg, cb, 64);
	    } else if(t==PT_LAVA && parts[i].life) {
		cr = parts[i].life * 2 + 0xE0;
		cg = parts[i].life * 1 + 0x50;
		cb = parts[i].life/2 + 0x10;
		if(cr>255) cr = 255;
		if(cg>192) cg = 192;
		if(cb>128) cb = 128;
		blendpixel(vid, nx, ny, cr, cg, cb, 255);
		blendpixel(vid, nx+1, ny, cr, cg, cb, 64);
		blendpixel(vid, nx-1, ny, cr, cg, cb, 64);
		blendpixel(vid, nx, ny+1, cr, cg, cb, 64);
		blendpixel(vid, nx, ny-1, cr, cg, cb, 64);
	    } else
		vid[ny*XRES+nx] = pcolors[t];
	}
}

/***********************************************************
 *                       SDL OUTPUT                        *
 ***********************************************************/

SDL_Surface *sdl_scrn;
int sdl_key;

void sdl_open(void)
{
    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK)<0) {
        fprintf(stderr, "Initializing SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);
    sdl_scrn=SDL_SetVideoMode(XRES*SCALE,YRES*SCALE + 40*SCALE,32,SDL_SWSURFACE);
    if(!sdl_scrn) {
        fprintf(stderr, "Creating window: %s\n", SDL_GetError());
        exit(1);
    }
}

void sdl_blit(int x, int y, int w, int h, unsigned int *src, int pitch)
{
    unsigned *dst,j;
#if (SCALE==2)
    unsigned i,k;
#endif
    if(SDL_MUSTLOCK(sdl_scrn))
        if(SDL_LockSurface(sdl_scrn)<0)
            return;
    dst=(unsigned *)sdl_scrn->pixels+y*sdl_scrn->pitch/4+x;
    for(j=0;j<h;j++) {
#if (SCALE==1)
        memcpy(dst, src, w*sizeof(unsigned));
        dst+=sdl_scrn->pitch/4;
#elif (SCALE==2)
        for(k=0;k<SCALE;k++) {
            for(i=0;i<w;i++) {
                dst[i*2]=src[i];
                dst[i*2+1]=src[i];
            }
            dst+=sdl_scrn->pitch/4;
        }
#else
#error Set SCALE to 1 or 2, please.
#endif
        src+=pitch/4;
    }
    if(SDL_MUSTLOCK(sdl_scrn))
        SDL_UnlockSurface(sdl_scrn);
    SDL_UpdateRect(sdl_scrn,0,0,0,0);
}

int frame_idx=0;
void dump_frame(unsigned int *src, int w, int h, int pitch)
{
    char frame_name[32];
    unsigned j,i,c;
    FILE *f;
    sprintf(frame_name,"frame%04d.ppm",frame_idx);
    f=fopen(frame_name,"w");
    fprintf(f,"P6\n%d %d\n255\n",w,h);
    for(j=0;j<h;j++) {
	for(i=0;i<w;i++) {
	    c=((src[i]&0xFF0000)>>16)|(src[i]&0x00FF00)|((src[i]&0x0000FF)<<16);
	    fwrite(&c,3,1,f);
	}
	src+=pitch/4;
    }
    fclose(f);
    frame_idx++;
}

int sdl_poll(void)
{
    SDL_Event event;
    sdl_key=0;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
	case SDL_KEYDOWN:
	    sdl_key=event.key.keysym.sym;
	    break;
        case SDL_QUIT:
	    return 1;
        }
    }
    return 0;
}

/***********************************************************
 *                    STATE MANAGEMENT                     *
 ***********************************************************/

void state_save(char *fn)
{
    unsigned sig = 0x54494853;
    FILE *f = fopen(fn, "w");
    fwrite(&sig, 1, 4, f);
    fwrite(parts, NPART, sizeof(particle), f);
    fwrite(bmap, 1, sizeof(bmap), f);
    fclose(f);
}

int state_load(char *fn)
{
    unsigned sig;
    FILE *f = fopen(fn, "r");
    fread(&sig, 1, 4, f);
    if(sig != 0x54494853) {
	fclose(f);
	return 1;
    }
    fread(parts, NPART, sizeof(particle), f);
    fread(bmap, 1, sizeof(bmap), f);
    fclose(f);
    memset(pv, 0, sizeof(pv));
    memset(vx, 0, sizeof(vx));
    memset(vy, 0, sizeof(vy));
    return 0;
}

/***********************************************************
 *                      FONT DRAWING                       *
 ***********************************************************/

#include "font.h"
#ifdef BGR
void drawpixel(unsigned *vid, int x, int y, int b, int g, int r, int a)
#else
void drawpixel(unsigned *vid, int x, int y, int r, int g, int b, int a)
#endif
{
    int t;
    if(x<0 || y<0 || x>=XRES || y>=YRES+40)
	return;
    if(a!=255) {
	t = vid[y*XRES+x];
	r = (a*r + (255-a)*((t>>16)&255)) >> 8;
	g = (a*g + (255-a)*((t>>8)&255)) >> 8;
	b = (a*b + (255-a)*(t&255)) >> 8;
    }
    vid[y*XRES+x] = (r<<16)|(g<<8)|b;
}

int drawchar(unsigned *vid, int x, int y, int c, int r, int g, int b, int a)
{
    int i, j, w, bn = 0, ba = 0;
    char *rp = font_data + font_ptrs[c];
    w = *(rp++);
    for(j=0; j<FONT_H; j++)
	for(i=0; i<w; i++) {
	    if(!bn) {
		ba = *(rp++);
		bn = 8;
	    }
	    drawpixel(vid, x+i, y+j, r, g, b, ((ba&3)*a)/3);
	    ba >>= 2;
	    bn -= 2;
	}
    return x + w;
}

int drawtext(unsigned *vid, int x, int y, char *s, int r, int g, int b, int a)
{
    int sx = x;
    for(;*s;s++) {
	if(*s == '\n') {
	    x = sx;
	    y += FONT_H+2;
	} else if(*s == '\b') {
	    switch(s[1]) {
	    case 'w':
		r = g = b = 255;
		break;
	    case 'g':
		r = g = b = 192;
		break;
	    case 'r':
		r = 255;
		g = b = 0;
		break;
	    case 'b':
		r = g = 0;
		b = 255;
		break;
	    }
	    s++;
	} else
	    x = drawchar(vid, x, y, *(unsigned char *)s, r, g, b, a);
    }
    return x;
}

int textwidth(char *s)
{
    int x = 0;
    for(;*s;s++)
	x += font_data[font_ptrs[(int)(*s)]];
    return x-1;
}

/***********************************************************
 *                      MAIN PROGRAM                       *
 ***********************************************************/

void draw_tool(unsigned *vid_buf, int b, int sl, int sr, unsigned pc)
{
    int x, y, i, j, c, cr, cg, cb;
    x = 2+32*(b/2);
    y = YRES+2+20*(b%2);

    if(b>=PT_NUM)
	x -= 512-XRES;

    if(b==27)
	for(j=1; j<15; j++)
	    for(i=1; i<27; i++) {
		if(!(i%2) && !(j%2))
		    vid_buf[XRES*(y+j)+(x+i)] = pc;
	    }
    else if(b==28)
	for(i=1; i<27; i++) {
	    cr = (int)(64.0f+63.0f*sin(i/4.0f));
	    cg = (int)(64.0f+63.0f*sin(i/4.0f+2.0f));
	    cb = (int)(64.0f+63.0f*sin(i/4.0f+4.0f));
	    pc = (cr<<16)|(cg<<8)|cb;
	    for(j=1; j<15; j++)
		vid_buf[XRES*(y+j)+(x+i)] = pc;
	}
    else if(b==29)
	for(j=1; j<15; j+=2)
	    for(i=1+(1&(j>>1)); i<27; i+=2)
		vid_buf[XRES*(y+j)+(x+i)] = pc;
    else if(b==30) {
	for(j=1; j<15; j+=2)
	    for(i=1+(1&(j>>1)); i<13; i+=2)
		vid_buf[XRES*(y+j)+(x+i)] = pc;
	for(j=1; j<15; j++)
	    for(i=14; i<27; i++)
		vid_buf[XRES*(y+j)+(x+i)] = pc;
    } else
	for(j=1; j<15; j++)
	    for(i=1; i<27; i++)
		vid_buf[XRES*(y+j)+(x+i)] = pc;

    if(b==30 || b==0)
	for(j=4; j<12; j++) {
	    vid_buf[XRES*(y+j)+(x+j+6)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x+j+7)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+21)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+22)] = 0xFF0000;
	}
    if(b==28)
	for(j=4; j<12; j++) {
	    vid_buf[XRES*(y+j)+(x+j)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x+j+1)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+15)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+16)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x+j+11)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x+j+12)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+26)] = 0xFF0000;
	    vid_buf[XRES*(y+j)+(x-j+27)] = 0xFF0000;
	}

    if(b>0 && b<PT_NUM) {
	c = (pcolors[b]&0xFF) + 3*((pcolors[b]>>8)&0xFF) + 2*((pcolors[b]>>16)&0xFF);
	if(c<512)
	    c = 255;
	else
	    c = 0;
	drawtext(vid_buf, x+14-textwidth(names[b])/2, y+4, names[b], c, c, c, 255);
    }

    if(b==sl || b==sr) {
	c = 0;
	if(b==sl)
	    c |= 0xFF0000;
	if(b==sr)
	    c |= 0x0000FF;
	for(i=0; i<30; i++) {
	    vid_buf[XRES*(y-1)+(x+i-1)] = c;
	    vid_buf[XRES*(y+16)+(x+i-1)] = c;
	}
	for(j=0; j<18; j++) {
	    vid_buf[XRES*(y+j-1)+(x-1)] = c;
	    vid_buf[XRES*(y+j-1)+(x+28)] = c;
	}
    }
}

void create_parts(int x, int y, int r, int c)
{
    int i, j;

    if(c == 27) {
	i = x / CELL;
	j = y / CELL;
	bmap[j][i] = 3;
	return;
    }
    if(c == 29) {
	i = x / CELL;
	j = y / CELL;
	bmap[j][i] = 2;
	return;
    }
    if(c == 30) {
	i = x / CELL;
	j = y / CELL;
	bmap[j][i] = 0;
	return;
    }
    if(c == 31) {
	i = x / CELL;
	j = y / CELL;
	bmap[j][i] = 1;
	return;
    }

    if(c == 0) {
	for(j=-r; j<=r; j++)
	    for(i=-r; i<=r; i++)
		if(i*i+j*j<=r*r)
		    delete_part(x+i, y+j);
	return;
    }

    for(j=-r; j<=r; j++)
	for(i=-r; i<=r; i++)
	    if(i*i+j*j<=r*r)
		create_part(-1, x+i, y+j, c);
}

void create_line(int x1, int y1, int x2, int y2, int r, int c)
{
    int cp=abs(y2-y1)>abs(x2-x1), x, y, dx, dy, sy;
    float e, de;
    if(cp) {
	y = x1;
	x1 = y1;
	y1 = y;
	y = x2;
	x2 = y2;
	y2 = y;
    }
    if(x1 > x2) {
	y = x1;
	x1 = x2;
	x2 = y;
	y = y1;
	y1 = y2;
	y2 = y;
    }
    dx = x2 - x1;
    dy = abs(y2 - y1);
    e = 0.0f;
    if(dx)
	de = dy/(float)dx;
    else
	de = 0.0f;
    y = y1;
    sy = (y1<y2) ? 1 : -1;
    for(x=x1; x<=x2; x++) {
	if(cp)
	    create_parts(y, x, r, c);
	else
	    create_parts(x, y, r, c);
	e += de;
	if(e >= 0.5f) {
	    y += sy;
	    if(c==31 || c==29 || c==27) {
		if(cp)
		    create_parts(y, x, r, c);
		else
		    create_parts(x, y, r, c);
	    }
	    e -= 1.0f;
	}
    }
}

void create_box(int x1, int y1, int x2, int y2, int c)
{
    int i, j;
    if(x1>x2) {
	i = x2;
	x2 = x1;
	x1 = i;
    }
    if(y1>y2) {
	j = y2;
	y2 = y1;
	y1 = j;
    }
    for(j=y1; j<=y2; j++)
	for(i=x1; i<=x2; i++)
	    create_parts(i, j, 1, c);
}

static void xor_pixel(int x, int y, unsigned *vid)
{
    int c;
    if(x<0 || y<0 || x>=XRES || y>=YRES)
	return;
    c = vid[y*XRES+x];
    c = (c&0xFF)+((c>>8)&0xFF)+((c>>16)&0xFF);
    if(c<384)
	vid[y*XRES+x] = 0xFFFFFF;
    else
	vid[y*XRES+x] = 0x000000;
}

void xor_line(int x1, int y1, int x2, int y2, unsigned *vid)
{
    int cp=abs(y2-y1)>abs(x2-x1), x, y, dx, dy, sy;
    float e, de;
    if(cp) {
	y = x1;
	x1 = y1;
	y1 = y;
	y = x2;
	x2 = y2;
	y2 = y;
    }
    if(x1 > x2) {
	y = x1;
	x1 = x2;
	x2 = y;
	y = y1;
	y1 = y2;
	y2 = y;
    }
    dx = x2 - x1;
    dy = abs(y2 - y1);
    e = 0.0f;
    if(dx)
	de = dy/(float)dx;
    else
	de = 0.0f;
    y = y1;
    sy = (y1<y2) ? 1 : -1;
    for(x=x1; x<=x2; x++) {
	if(cp)
	    xor_pixel(y, x, vid);
	else
	    xor_pixel(x, y, vid);
	e += de;
	if(e >= 0.5f) {
	    y += sy;
	    e -= 1.0f;
	}
    }
}

char *tag = "(c) 2008 Stanislaw Skowronek";

int main(int argc, char *argv[])
{
    unsigned *vid_buf = calloc(XRES*(YRES+40), sizeof(unsigned));
    int i, j, vs = 0;
    int x, y, b = 0, sl=1, sr=0, c, lb = 0, lx = 0, ly = 0, lm = 0, tx, ty;
    int da = 0, db = 0, it = 2047;
    SDLMod mk = 0;

    parts = calloc(sizeof(particle), NPART);
    for(i=0; i<NPART-1; i++)
	parts[i].life = i+1;
    parts[NPART-1].life = -1;
    pfree = 0;

    make_kernel();

    sdl_open();
    while(!sdl_poll()) {
	for(i=0; i<YRES/CELL; i++) {
	    pv[i][1] = pv[i][2]*0.9f;
	    pv[i][2] = pv[i][3]*0.9f;
	    pv[i][XRES/CELL-2] = pv[i][XRES/CELL-3]*0.9f;
	    pv[i][XRES/CELL-1] = pv[i][XRES/CELL-2]*0.9f;
	    vx[i][0] = vx[i][1]*0.7f;
	    vx[i][1] = vx[i][2]*0.7f;
	    vx[i][XRES/CELL-2] = vx[i][XRES/CELL-3]*0.7f;
	    vx[i][XRES/CELL-1] = vx[i][XRES/CELL-2]*0.7f;
	    vy[i][0] = vy[i][1]*0.7f;
	    vy[i][1] = vy[i][2]*0.7f;
	    vy[i][XRES/CELL-2] = vy[i][XRES/CELL-3]*0.7f;
	    vy[i][XRES/CELL-1] = vy[i][XRES/CELL-2]*0.7f;
	}
	for(i=0; i<XRES/CELL; i++) {
	    pv[1][i] = pv[2][i]*0.9f;
	    pv[2][i] = pv[3][i]*0.9f;
	    pv[YRES/CELL-2][i] = pv[YRES/CELL-3][i]*0.9f;
	    pv[YRES/CELL-1][i] = pv[YRES/CELL-2][i]*0.9f;
	    vx[0][i] = vx[1][i]*0.7f;
	    vx[1][i] = vx[2][i]*0.7f;
	    vx[YRES/CELL-2][i] = vx[YRES/CELL-3][i]*0.7f;
	    vx[YRES/CELL-1][i] = vx[YRES/CELL-2][i]*0.7f;
	    vy[0][i] = vy[1][i]*0.7f;
	    vy[1][i] = vy[2][i]*0.7f;
	    vy[YRES/CELL-2][i] = vy[YRES/CELL-3][i]*0.7f;
	    vy[YRES/CELL-1][i] = vy[YRES/CELL-2][i]*0.7f;
	}

	for(j=1; j<YRES/CELL; j++)
	    for(i=1; i<XRES/CELL; i++)
		if(bmap[j][i]==1) {
		    vx[j][i] = 0.0f;
		    vx[j][i-1] = 0.0f;
		    vy[j][i] = 0.0f;
		    vy[j-1][i] = 0.0f;
		}

	update_air();
	draw_air(vid_buf);
	update_particles(vid_buf);

	memset(vid_buf+(XRES*YRES), 0, sizeof(unsigned)*XRES*40);
	for(b=0; b<PT_NUM; b++)
	    draw_tool(vid_buf, b, sl, sr, pcolors[b]);
	draw_tool(vid_buf, 27, sl, sr, 0x808080);
	draw_tool(vid_buf, 28, sl, sr, 0x808080);
	draw_tool(vid_buf, 29, sl, sr, 0x808080);
	draw_tool(vid_buf, 30, sl, sr, 0x808080);
	draw_tool(vid_buf, 31, sl, sr, 0x808080);

	if(sdl_key=='q' || sdl_key==SDLK_ESCAPE)
	    break;
	if(sdl_key=='s')
	    state_save("save.bin");
	if(sdl_key=='l')
	    state_load("save.bin");
	if(sdl_key=='p')
	    dump_frame(vid_buf, XRES, YRES, XRES*4);
	if(sdl_key=='v')
	    vs = !vs;
	if(vs)
	    dump_frame(vid_buf, XRES, YRES, XRES*4);

	b = SDL_GetMouseState(&x, &y);
	mk = SDL_GetModState();

	if(y >= SCALE*YRES) {
	    tx = (x/SCALE)/32;
	    ty = ((y/SCALE)-YRES)/20;
	    c = 2*tx+ty;
	    if((c>=0 && c<PT_NUM) || (c>=27 && c<32)) {
		db = c;
		if(da < 51)
		    da ++;
	    } else {
		if(da > 0)
		    da --;
	    }
	} else
	    if(da > 0)
		da --;

	if(b) {
	    if(it > 50)
		it = 50;
	    x /= SCALE;
	    y /= SCALE;
	    if(y >= YRES) {
		if(!lb) {
		    tx = x/32;
		    ty = (y-YRES)/20;
		    c = 2*tx+ty;
		    if(c>=PT_NUM) {
			tx = (x+512-XRES)/32;
			c = 2*tx+ty;
		    }
		    if((c>=0 && c<PT_NUM) || (c>=29 && c<32) || (c==27)) {
			if(b&1)
			    sl = c;
			else
			    sr = c;
		    }
		    if(c==28 && (b&1)) {
			memset(pv, 0, sizeof(pv));
			memset(vx, 0, sizeof(vx));
			memset(vy, 0, sizeof(vy));
			memset(bmap, 0, sizeof(bmap));
			memset(parts, 0, sizeof(particle)*NPART);
			for(i=0; i<NPART-1; i++)
			    parts[i].life = i+1;
			parts[NPART-1].life = -1;
			pfree = 0;
		    }
		    lb = 0;
		}
	    } else {
		c = (b&1) ? sl : sr;
		if(lb) {
		    if(lm == 1) {
			xor_line(lx, ly, x, y, vid_buf);
		    } else if(lm == 2) {
			xor_line(lx, ly, lx, y, vid_buf);
			xor_line(lx, y, x, y, vid_buf);
			xor_line(x, y, x, ly, vid_buf);
			xor_line(x, ly, lx, ly, vid_buf);
		    } else {
			create_line(lx, ly, x, y, 2, c);
			lx = x;
			ly = y;
		    }
		} else {
		    if(mk & (KMOD_LSHIFT|KMOD_RSHIFT)) {
			lx = x;
			ly = y;
			lb = b;
			lm = 1;
		    } else if(mk & (KMOD_LCTRL|KMOD_RCTRL)) {
			lx = x;
			ly = y;
			lb = b;
			lm = 2;
		    } else {
		        create_parts(x, y, 2, c);
			lx = x;
			ly = y;
			lb = b;
			lm = 0;
		    }
		}
	    }
	} else {
	    if(lb && lm) {
		x /= SCALE;
		y /= SCALE;
		c = (lb&1) ? sl : sr;
		if(lm == 1)
		    create_line(lx, ly, x, y, 2, c);
		else
		    create_box(lx, ly, x, y, c);
		lm = 0;
	    }
	    lb = 0;
	}

	if(da)
	    switch(db) {
	    case 27:
		drawtext(vid_buf, 16, YRES-24, "Wall. Blocks most particles but lets liquids through.", 255, 255, 255, da*5);
		break;
	    case 28:
		drawtext(vid_buf, 16, YRES-24, "Erases all particles and walls.", 255, 255, 255, da*5);
		break;
	    case 29:
		drawtext(vid_buf, 16, YRES-24, "Wall. Absorbs particles but lets air currents through.", 255, 255, 255, da*5);
		break;
	    case 30:
		drawtext(vid_buf, 16, YRES-24, "Erases walls.", 255, 255, 255, da*5);
		break;
	    case 31:
		drawtext(vid_buf, 16, YRES-24, "Wall. Indestructible. Blocks everything.", 255, 255, 255, da*5);
		break;
	    default:
		drawtext(vid_buf, 16, YRES-24, descs[db], 255, 255, 255, da*5);
	    }

	if(it) {
	    it--;
	    drawtext(vid_buf, 16, 20, it_msg, 255, 255, 255, it>51?255:it*5);
	}

	sdl_blit(0, 0, XRES, YRES+40, vid_buf, XRES*4);
    }
    return 0;
}
