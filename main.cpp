#include <iostream>
#include <stdio.h>
#include <string>
#include <array>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
using namespace std;
//Screen dimension constants
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 360;
class GameObject;

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

//The window renderer
SDL_Renderer* gRenderer = NULL;

std::array<GameObject*,5> gObjects;

bool gPause = false;

Uint64 gNOW;

Uint64 gLAST;

double gDT;

class LTexture
{
	public:
		//Initializes variables
		LTexture();

		//Deallocates memory
		~LTexture();

		//Loads image at specified path
		bool loadFromFile( std::string path );
		
		#ifdef _SDL_TTF_H
		//Creates image from font string
		bool loadFromRenderedText( std::string textureText, SDL_Color textColor );
		#endif
		
		//Deallocates texture
		void free();

		//Set color modulation
		void setColor( Uint8 red, Uint8 green, Uint8 blue );

		//Set blending
		void setBlendMode( SDL_BlendMode blending );

		//Set alpha modulation
		void setAlpha( Uint8 alpha );
		
		//Renders texture at given point
		void render( int x, int y, SDL_Rect* clip = NULL, double angle = 0.0, SDL_Point* center = NULL, SDL_RendererFlip flip = SDL_FLIP_NONE );

		//Gets image dimensions
		int getWidth();
		int getHeight();

	private:
		//The actual hardware texture
		SDL_Texture* mTexture;

		//Image dimensions
		int mWidth;
		int mHeight;
};

class Rect{

	public:

		Rect(int x, int y, int w, int h){
			fillRect.x = x;
			fillRect.y = y;
			fillRect.w = w;
			fillRect.h = h;
		}

		void draw(){
			SDL_SetRenderDrawColor( gRenderer, 0xFF, 0x00, 0x00, 0xFF ); 
			SDL_RenderFillRect( gRenderer, &fillRect );
		}
	
	private:
		SDL_Rect fillRect;

};

class InputComponent
{
	public:
		virtual ~InputComponent(){};
		virtual void update(GameObject& obj, SDL_Event e) = 0;
};

class PhysicsComponent
{
	public:
		virtual ~PhysicsComponent(){};
		virtual void update(GameObject& obj, std::array<GameObject*,5> objects) = 0;
};

class GraphicsComponent
{
	public:
		virtual ~GraphicsComponent(){};
		virtual void update(GameObject& obj) = 0;
};

class GameObject
{
	public:
		//coords
		int x,y;

		int start = 0;
		//Velocity (in this game only one axis moves per object)
		double vel;
		//Timer
		Uint64 startTime;

		Uint64 currentTime = 0;


		GameObject(InputComponent* input,
             PhysicsComponent* physics,
             GraphicsComponent* graphics,
			 int x, int y, double vel, int start)
			: input_(input),
				physics_(physics),
				graphics_(graphics),
				x(x),y(y),vel(vel),start(start)
				{
					startTime = SDL_GetTicks();
				};

		~GameObject(){
			x = 0;
			y = 0;
			vel = 0;
			startTime = 0;
			delete input_;
			delete physics_;
			delete graphics_;
		};

		void handleInput(SDL_Event e){
			input_->update(*this, e);
		}

		void update(std::array<GameObject*,5> objects){
			physics_->update(*this, objects);
		}
		
		void render(){
			graphics_->update(*this);
		}

		InputComponent* getInput(){
			return input_;
		}

		PhysicsComponent* getPhysics(){
			return physics_;
		}

		GraphicsComponent* getGraphics(){
			return graphics_;
		}

		private:
		InputComponent* input_;
		PhysicsComponent* physics_;
		GraphicsComponent* graphics_;

};
class PipeInputComponent : public InputComponent
{
	public:
		virtual void update(GameObject& obj, SDL_Event e){
			//No input = does nothing
			return;
		}
};

class PipePhysicsComponent : public PhysicsComponent
{
	public:
		virtual void update(GameObject& obj, std::array<GameObject*,5> objects){
			//Updates PosX
			if(!gPause){
				obj.currentTime = SDL_GetTicks();
			}
			double dt = (double)(obj.currentTime - obj.startTime)/1000;
			obj.x =obj.start + obj.vel*dt;
			if(obj.x < 0){
				obj.start = SCREEN_WIDTH;
				obj.startTime = SDL_GetTicks();
				obj.y = rand() % (SCREEN_HEIGHT -150) +50;
			}
			//std::cout << "dt: " << dt << " y: " << obj.x << endl;
			//Checks for collision with bird 
		}
	
};

class PipeGraphicsComponent : public GraphicsComponent
{
	public:
		~PipeGraphicsComponent(){
			w = 0;
			gap = 0;
			delete top;
			delete bottom;
			top = NULL;
			bottom = NULL;
		}

		virtual void update(GameObject& obj){
			top = new Rect(obj.x, 0, w, obj.y );
			bottom = new Rect(obj.x, obj.y + gap, w , SCREEN_HEIGHT - obj.y - gap);
			top->draw();
			bottom->draw();
			//Renders two pipes with set distance between them using fillRect
		}

		int getWidth(){
			return w;
		}

		int getGap(){
			return gap;
		}

	private:
		int w = 40;
		int gap = 70;
		Rect* top;
		Rect* bottom;
};

GameObject* createPipe(int x, int y, double vel, int start){
	return new GameObject(new PipeInputComponent(), new PipePhysicsComponent(), new PipeGraphicsComponent(), x, y, vel, start);
}

class BirdInputComponent : public InputComponent
{
	public:
		virtual void update(GameObject& obj, SDL_Event e){

			if(e.type == SDL_KEYDOWN){
				obj.vel = -250;
				obj.startTime = SDL_GetTicks(); // Prob should revise this in some way
				obj.start = obj.y;
			}
		}
};

class BirdPhysicsComponent : public PhysicsComponent
{
	public:
		~BirdPhysicsComponent(){};

		virtual void update(GameObject& obj,std::array<GameObject*,5> objects){
			//Adds velocity and updates PosY
			if(!gPause){
				obj.currentTime = SDL_GetTicks();
			}
			double dt = (double)(obj.currentTime - obj.startTime)/1000;
			obj.y = obj.start + obj.vel*dt + 20*dt*dt*ACC;
			//Checks for collision with pipes and wall
		if( (obj.y - RAD < 0) || (obj.y + RAD > SCREEN_HEIGHT) ){
						
						if(obj.y - RAD < 0 ){
							obj.y = RAD;
							obj.start = RAD;
							obj.vel = 0;
							obj.startTime = SDL_GetTicks();
						}
						else{
							obj.y = SCREEN_HEIGHT - RAD;
							obj.start = SCREEN_HEIGHT - RAD;
							obj.vel = 0;
							obj.startTime = SDL_GetTicks();
						}	
					}
			for(int i = 1; i < objects.size(); i++){
				PipeGraphicsComponent* P = (PipeGraphicsComponent*)objects[i]->getGraphics();				
				if(obj.x + RAD < objects[i]->x){
					continue;
				}
				if(obj.x - RAD > objects[i]->x + P->getWidth()){
					continue;
				}
				if(obj.y - RAD > objects[i]->y && obj.y + RAD < objects[i]->y + P->getGap() ){
					continue;
				}
				//cout << "COLLISION" << endl;
				gPause = true;
				



				



			}
		}



	private:
		const double ACC = 20;
		const int RAD = 10;
};

class BirdGraphicsComponent : public GraphicsComponent
{
	public:
		BirdGraphicsComponent(){
			Bird.loadFromFile("Cir10.png");
		}

		~BirdGraphicsComponent(){
			Bird.free();
		}
		virtual void update(GameObject& obj){
			//Renders bird texture from image with LTexture::render
			Bird.render( obj.x - RAD, obj.y -RAD);;
		}

		int getRad(){
			return RAD;
		}
	
	private:
		const int RAD = 10;
		LTexture Bird;
};

GameObject* createBird(int x, int y, double vel, int start){
	return new GameObject(new BirdInputComponent(), new BirdPhysicsComponent(), new BirdGraphicsComponent(), x, y, vel, start);
}

//Texture wrapper class

class Circle{
	
	public:
		//Dims
		static const int RAD = 10;

		//Max axis velocity
		static const int CIR_VEL = 0;

		static const int MAX_VEL = 5;

		double ACC = 10;

		//Init Circle
		Circle();

		//Deallocate memory
		//~Circle();		

		//Handle velocity change
		void handleEvent( SDL_Event& e);

		//Moves circle
		void move();

		//Renders circle
		void render();

	private:
		//Position of da circle
		int mPosX, mPosY;

		//Velocity of the circle
		int mVelX, mVelY;

};

//Starts up SDL and creates window
bool init();

//Loads media
bool loadMedia();

//Frees media and shuts down SDL
void close();






LTexture::LTexture()
{
	//Initialize
	mTexture = NULL;
	mWidth = 0;
	mHeight = 0;
}

LTexture::~LTexture()
{
	//Deallocate
	free();
}


bool LTexture::loadFromFile( std::string path )
{
	//Get rid of preexisting texture
	free();

	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( path.c_str() );

	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", path.c_str(), IMG_GetError() );
	}
	else
	{
		//Color key image
		SDL_SetColorKey( loadedSurface, SDL_TRUE, SDL_MapRGB( loadedSurface->format, 0, 0xFF, 0xFF ) );

		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( gRenderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", path.c_str(), SDL_GetError() );
		}
		else
		{
			//Get image dimensions
			mWidth = loadedSurface->w;
			mHeight = loadedSurface->h;
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	//Return success
	mTexture = newTexture;
	return mTexture != NULL;
}

#ifdef _SDL_TTF_H
bool LTexture::loadFromRenderedText( std::string textureText, SDL_Color textColor )
{
	//Get rid of preexisting texture
	free();

	//Render text surface
	SDL_Surface* textSurface = TTF_RenderText_Solid( gFont, textureText.c_str(), textColor );
	if( textSurface == NULL )
	{
		printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        mTexture = SDL_CreateTextureFromSurface( gRenderer, textSurface );
		if( mTexture == NULL )
		{
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
		}
		else
		{
			//Get image dimensions
			mWidth = textSurface->w;
			mHeight = textSurface->h;
		}

		//Get rid of old surface
		SDL_FreeSurface( textSurface );
	}
	
	//Return success
	return mTexture != NULL;
}
#endif

LTexture gCircleTexture;

void LTexture::free()
{
	//Free texture if it exists
	if( mTexture != NULL )
	{
		SDL_DestroyTexture( mTexture );
		mTexture = NULL;
		mWidth = 0;
		mHeight = 0;
	}
}

void LTexture::setColor( Uint8 red, Uint8 green, Uint8 blue )
{
	//Modulate texture rgb
	SDL_SetTextureColorMod( mTexture, red, green, blue );
}

void LTexture::setBlendMode( SDL_BlendMode blending )
{
	//Set blending function
	SDL_SetTextureBlendMode( mTexture, blending );
}
		
void LTexture::setAlpha( Uint8 alpha )
{
	//Modulate texture alpha
	SDL_SetTextureAlphaMod( mTexture, alpha );
}

void LTexture::render( int x, int y, SDL_Rect* clip, double angle, SDL_Point* center, SDL_RendererFlip flip )
{
	//Set rendering space and render to screen
	SDL_Rect renderQuad = { x, y, mWidth, mHeight };

	//Set clip rendering dimensions
	if( clip != NULL )
	{
		renderQuad.w = clip->w;
		renderQuad.h = clip->h;
	}

	//Render to screen
	SDL_RenderCopyEx( gRenderer, mTexture, clip, &renderQuad, angle, center, flip );
}

int LTexture::getWidth()
{
	return mWidth;
}

int LTexture::getHeight()
{
	return mHeight;
}

Circle::Circle(){
	mPosX = RAD + SCREEN_WIDTH/2; //middle of circle in middle of screen
	mPosY = RAD;

	mVelX = 0;
	mVelY = 0;

}

void Circle::handleEvent( SDL_Event& e){
	if(e.type == SDL_KEYDOWN){
		gLAST = SDL_GetTicks();
		mVelY = -30;
	}
	/*
	double dt = 0;
	//If a key was pressed
	//if(e.type == SDL_KEYDOWN && e.key.repeat == 0){
	//}
	if(e.type == SDL_KEYDOWN){
		if(!gLAST){
			gLAST = SDL_GetTicks();
		}
		//gLAST = gNOW;
		gNOW = SDL_GetTicks();
		dt = (double)((gNOW - gLAST))/1000;
		cout << mVelX << endl;
		//cout << (int)(ACC*dt) << endl;
		//Adjust the velocity
		switch( e.key.keysym.sym){
            case SDLK_UP: mVelY -= (int)(ACC*dt)+1; mVelY -= CIR_VEL;  break;
            case SDLK_DOWN: mVelY += (int)(ACC*dt)+1; mVelY += CIR_VEL; break;
            case SDLK_LEFT: mVelX -= (int)(ACC*dt)+1; mVelX -= CIR_VEL; break;
            case SDLK_RIGHT: mVelX += (int)(ACC*dt)+1; mVelX += CIR_VEL; break;
			
        }


	}

	
	else if( e.type == SDL_KEYUP && e.key.repeat == 0 ){
        //Adjust the velocity
		gLAST = 0;
		switch( e.key.keysym.sym){
            case SDLK_UP: mVelY = 0; mVelY += CIR_VEL;  break;
            case SDLK_DOWN: mVelY = 0; mVelY -= CIR_VEL; break;
            case SDLK_LEFT: mVelX = 0; mVelX += CIR_VEL; break;
            case SDLK_RIGHT: mVelX = 0; mVelX -= CIR_VEL; break;
		}
    }

	if( abs(mVelX) > MAX_VEL ){
		if(mVelX > 0){
			mVelX = MAX_VEL;
		}
		else{
			mVelX = -MAX_VEL;
		}
	}

	if( abs(mVelY) > MAX_VEL ){
		if(mVelY > 0){
			mVelY = MAX_VEL;
		}
		else{
			mVelY = -MAX_VEL;
		}
	}
	*/
}

void Circle::move(){
	//cout << gDT << endl;
	mPosX += mVelX;

	if( (mPosX - RAD < 0) || (mPosX + RAD > SCREEN_WIDTH) ){
		mPosX -= mVelX;
		mVelX = 0;
		cout << mVelX << endl;
	}
	//mVelY += (ACC * gDT);
	mPosY +=mVelY*gDT + 0.5*gDT*ACC*gDT*12 ;
	
	if( (mPosY - RAD < 0) || (mPosY + RAD > SCREEN_HEIGHT) ){
		
		if(mPosY - RAD < 0 ){
			mPosY = RAD;
			mVelY = 0;
			cout << mPosY << endl;
		}
		else{
			mPosY = SCREEN_HEIGHT - RAD;
			mVelY = 0;
		}
		
		gLAST = SDL_GetTicks();

		
	}

}

void Circle::render(){
	gCircleTexture.render( mPosX - RAD, mPosY -RAD);
}

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 )
	{
		printf( "SDL could not initialize! SDL Error: %s\n", SDL_GetError() );
		success = false;
	}
	else
	{
		//Set texture filtering to linear
		if( !SDL_SetHint( SDL_HINT_RENDER_SCALE_QUALITY, "1" ) )
		{
			printf( "Warning: Linear texture filtering not enabled!" );
		}

		//Create window
		gWindow = SDL_CreateWindow( "SDL Tutorial", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if( gWindow == NULL )
		{
			printf( "Window could not be created! SDL Error: %s\n", SDL_GetError() );
			success = false;
		}
		else
		{
			//Create vsynced renderer for window
			gRenderer = SDL_CreateRenderer( gWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC );
			if( gRenderer == NULL )
			{
				printf( "Renderer could not be created! SDL Error: %s\n", SDL_GetError() );
				success = false;
			}
			else
			{
				//Initialize renderer color
				SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );

				//Initialize PNG loading
				int imgFlags = IMG_INIT_PNG;
				if( !( IMG_Init( imgFlags ) & imgFlags ) )
				{
					printf( "SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
					success = false;
				}
			}
		}
	}

	return success;
}

bool loadMedia()
{
	//Loading success flag
	bool success = true;
	//Load dot texture
	if( !gCircleTexture.loadFromFile( "Cir10.png" ) )
	{
		printf( "Failed to load circle texture!\n" );
		success = false;
	}

	return success;
}

void close()
{	for(int i = 0; i < 5; i++){
		delete gObjects[i];
		gObjects[i] = NULL;

	}
	gCircleTexture.free();
	//Destroy window	
	SDL_DestroyRenderer( gRenderer );
	SDL_DestroyWindow( gWindow );
	gWindow = NULL;
	gRenderer = NULL;

	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

int main( int argc, char* args[] )
{
	//Start up SDL and create window
	if( !init() )
	{
		printf( "Failed to initialize!\n" );
	}
	else
	{
		//Load media
		if( !true )
		{
			printf( "Failed to load media!\n" );
		}
		else
		{	
			//Main loop flag
			bool quit = false;
			//Reset Timer
			gLAST = SDL_GetTicks();
			//Event handler
			SDL_Event e;
			//Init random seed
			srand(time(0));

			GameObject* B = createBird(SCREEN_WIDTH / 2, 0, 0, 0);
			//Bird is first in array
			gObjects[0] = B;

			for(int i = 1; i < 5; i++){
				gObjects[i] = createPipe(0,
								rand() % (SCREEN_HEIGHT -150) +50,
								-100,
								SCREEN_WIDTH + SCREEN_WIDTH / 4 * (i-1)	);
			}
		






		/*
			PP-> x = SCREEN_WIDTH+SCREEN_WIDTH/4;
			PP-> y = rand() % (SCREEN_HEIGHT -150) +100;
			PP-> vel = -90;
			PP-> start = SCREEN_WIDTH+SCREEN_WIDTH/4;		*/
			Circle circle;


			//While application is running
			while( !quit )
			{
				//Handle events on queue
				while( SDL_PollEvent( &e ) != 0 )
				{
					//User requests quit
					if( e.type == SDL_QUIT )
					{
						quit = true;
					}
					B->handleInput(e);
				}
				//get current time
				//NOT PERMANENT
				if(gPause){
					gObjects[0]->vel = 0;
					gObjects[0]->start = 0;
					gObjects[0]->startTime = SDL_GetTicks();
					/*cout << "deleting bird" << endl;
					delete B;
					delete gObjects[0];
					GameObject* B = createBird(SCREEN_WIDTH / 2, 0, 0, 0);
					//Bird is first in array
					gObjects[0] = B;
					cout << "delete pipes" << endl;*/
					for(int i = 1; i < 5; i++){
						gObjects[i]->start = SCREEN_WIDTH + SCREEN_WIDTH / 4 * (i-1);
						gObjects[i]->y = rand() % (SCREEN_HEIGHT -150) + 50;
						gObjects[i]->startTime = SDL_GetTicks();
						/*delete gObjects[i];
						gObjects[i] = createPipe(0,
										rand() % (SCREEN_HEIGHT -100) +50,
										-90,
										SCREEN_WIDTH + SCREEN_WIDTH / 4 * (i-1)	);*/
					}		
					gPause = false;			

				}
				
				gNOW = SDL_GetTicks();
				gDT = (double)(gNOW - gLAST)/1000;
				for(int i = 0; i < 5; i++){
					gObjects[i]->update(gObjects);
				}


				//B->update(gObjects);

				//P->update(gObjects);
				//PP->update(gObjects);
				//Clear screen
				SDL_SetRenderDrawColor( gRenderer, 0xFF, 0xFF, 0xFF, 0xFF );
				SDL_RenderClear( gRenderer );
				for(int i = 0; i < 5; i++){
					gObjects[i]->render();
				}
				//B->render();
				//P->render();
				//PP->render();
				//circle.render();
				//Update screen
				SDL_RenderPresent( gRenderer );
			}
		}
	}

	//Free resources and close SDL
	close();

	return 0;
}