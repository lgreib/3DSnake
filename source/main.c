#include<string.h>//memset memcpy
#include<stdio.h> //printf
#include<stdlib.h>//malloc rand
#include <3ds.h>

enum object {WALL=-2, FOOD=-1, NOTHING=0}; //Snake parts are all >=1
enum statuses {PENDING, NORMAL, EATEN, DEAD};
enum directions {UP, DOWN, LEFT, RIGHT};

static int map[48][80];
static int length, state, score, speed, direction, lastdirection;
static u8* myfb;

void initgame();
void spawnfood();
void move();
void drawmap();
void drawinfo();

int main()
{
  // Initializations
  srvInit();
  aptInit();
  gfxInitDefault();
  gfxSet3D(false);
  consoleInit(GFX_BOTTOM, NULL);
  hidInit(NULL);
  
  u32 kDown, kHeld;
  
  //No stunning 60 fps here; create a single buffer to copy to both render and prerender frame buffers
  myfb = (u8*)malloc(240*400*3);
  
  initgame();
  speed = 5;
  drawmap();
  drawinfo();
  state = PENDING;
  
  int counter = 1000;
  int paused = 0;
  while (aptMainLoop())
  {
    // Wait for next frame
    gspWaitForVBlank();
	
    // Read buttons
    hidScanInput();
    kDown = hidKeysDown();
	kHeld = hidKeysHeld();
	
	if (kDown & KEY_SELECT){
      break;
    }	
	
	if(kDown & KEY_X || kDown & KEY_Y){
	    if(kDown & KEY_X){
		    speed++;
			if(speed > 10){
			    speed = 10;
			}
		}
		else{
		    speed--;
			if(speed < 1){
			    speed = 1;
			}
		}
		drawinfo();
	}
	
	if (kDown & KEY_START){
      if(paused){
	      paused = 0;
	  }
	  else{
	      paused = 1;
	  }
    }	
	
	if(paused){
	    continue;
	}
	
	if(state == PENDING || state == DEAD){
	    if(kDown & KEY_A){
		    counter = 1000;
		    initgame();
			spawnfood();
			drawmap();
			drawinfo();
		}
		continue;
	}

	if(kHeld & KEY_UP && lastdirection != DOWN){
	    direction = UP;
	}
	if(kHeld & KEY_DOWN && lastdirection != UP){
	    direction = DOWN;
	}
	if(kHeld & KEY_LEFT && lastdirection != RIGHT){
	    direction = LEFT;
	}
	if(kHeld & KEY_RIGHT && lastdirection != LEFT){
	    direction = RIGHT;
	}
	
	if(counter == 0){
	    counter = 1000;
	}
	if(counter%(11-speed) == 0){
	    lastdirection = direction;
		move();
		if(state != NORMAL){//dead or eaten
		    if(state == EATEN){
			    length++;
				score += speed;
				state = NORMAL;
				spawnfood();
			}
			drawinfo();
		}
		drawmap();
	}
	counter--;
  }

  //Exit
  free(myfb);
  gfxExit();
  hidExit();
  aptExit();
  srvExit();
  return 0;
}

void initgame(){
	int x, y;
	for(x = 0; x < 48; x++){
	    for(y = 0; y < 80; y++){
		    if(x == 0 || x == 47 || y == 0 || y == 79){
			    map[x][y] = WALL;
			}
			else{
			    map[x][y] = NOTHING;
			}
		}
	}
	map[24][40] = 1;
	length = 1;
	score = 0;
	state = NORMAL;
	direction = UP;
	lastdirection = UP;
}


void move(){
    int x, y;
	int brk = 0;
    for(x = 0; x < 48 &&!brk; x++){//Move the head
	    for(y = 0; y < 80 &&!brk; y++){
			if(map[x][y] == length){
			    int newx = x + (direction == DOWN ? -1 : direction == UP ? 1 : 0);
				int newy = y + (direction == LEFT ? -1 : direction == RIGHT ? 1 : 0);
				int next = map[newx][newy];
				if(next == WALL || next > 1){
					state = DEAD;
				}
				else if(next == FOOD){
				    state = EATEN;
				}
                else{
				    state = NORMAL;
				}
				map[newx][newy] = length+1;
				brk=1;
			}
		}
	}
	if(state != EATEN){//Decrement the body parts
	    for(x = 0; x < 48; x++){
	        for(y = 0; y < 80; y++){
		        if(map[x][y] > 0){
			        map[x][y]--;
			    }
		    }
	    }
	}
}

void spawnfood(){
    int placed = 0;
    int x = (rand() % 47) + 1;
	int y = (rand() % 79) + 1;
    
	if(map[x][y] == NOTHING){
	    map[x][y] = FOOD;
	    placed = 1;
	}
	//Place it at the next nearest coordinate if the spot is occupied
	int i,j;
	for(i=x;i<48&&!placed;i++){
	    for(j=y;j<80&&!placed;j++){
		    if(map[i][j] == NOTHING){
			    map[i][j] = FOOD;
			    placed =1;
			}
		}
	}
	for(i=x;i>0&&!placed;i--){
	    for(j=y;j>0&&!placed;j--){
		    if(map[i][j] == NOTHING){
			    map[i][j] = FOOD;
			    placed =1;
			}
		}
	}
}

void drawmap(){
    //Framebuffer is oriented like this. Each map coordinate corresponds to 5 square pixels.
	/*      0,0 _______
	analog     |       |
		       |       |
		       |       |
		       |       |
	buttons	   |_______|240,400
	*/
	memset(myfb, 0, 240*400*3); // clear the buffer
	int x, y, i, j;
    for(x = 0; x < 48; x++){
        for(y=0; y<80; y++){
	        if(map[x][y] != NOTHING){
			    for(i=0;i<5;i++){ //Draw a 5x5 square
			        for(j=0;j<5;j++){
				        int pixeloffset = (x*5+i)*3+(y*5*240+j*240)*3;
				        myfb[pixeloffset] = (state==DEAD && map[x][y]>0) ? 0x00:0xFF;   //B
					    myfb[pixeloffset+1] = (state==DEAD && map[x][y]>0) ? 0x00:0xFF; //G
					    myfb[pixeloffset+2] = 0xFF; //R
				    }
			    }  
		    }
	    }
    }
	//Copy image to both top-left screen buffers
	u8* fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(fb, myfb, 240*400*3);
    gfxFlushBuffers();
    gfxSwapBuffers();
	fb = gfxGetFramebuffer(GFX_TOP, GFX_LEFT, NULL, NULL);
	memcpy(fb, myfb, 240*400*3);
	gfxFlushBuffers();
    gfxSwapBuffers();
}

void drawinfo(){
     consoleClear();
	 printf("Score: %d", score);
	 printf("\nSpeed: %d", speed);
	 
	 printf("\n\n---Controls---");
	 printf("\nA: Start Game");
	 printf("\nX: Increase Speed");
	 printf("\nY: Decrease Speed");
	 printf("\nStart: Pause");
	 printf("\nSelect: Exit");
     
	 if(state == DEAD){
         printf("\n\n\nSnake?\n\nSnake?!\n\nSNAAAAAKE!!!");
	 }
	gfxFlushBuffers();
	gfxSwapBuffers();
}