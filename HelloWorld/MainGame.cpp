#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;

// 1A) Create GameState struct in the global scope  and initiate it
// this game state is to remember the overall state of the game
struct GameState {
	float timer = 0; 
	//int spriteId = 0; store id of the current sprite
	int score{ 0 };
}; 

GameState gameState;

//1B. Create Game Objects type
/* the GameObject struc is provided by PlayManaer for representing interacting objects in a typical game
* every GameObject has a set of common properties and PlayManager has a range of useful functions for managing them.
* This GameObject has already been created in Play.h
*/
//struct GameObject {
//	int type;
//	int oldType;
//	int spriteId;
//	Point2D pos;
//	Point2D oldPos;
//	Vector2D velocity;
//	Vector2D acceleration;
//	float rotation;
//	float rotSpeed;
//	float oldRot;
//	int frame;
//	float framePos;
//	float animSpeed;
//	int radius;
//	float scale;
//	int order;
//	int lastFrameUpdated;
//}

// 1B) create an emum to hold all the interactive objects
// the first item in this enum TYPE_NULL is set to -1 because PlayManager uses -1 to represetnt uninitialised objects.
// then the next object TYPE_AGENT8 will be given a value of 1

enum GameObjectType
{
	TYPE_NULL = -1,
	TYPE_AGENT8, // a robot spider
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED,
};


// 2) Declare game functions for to be called in  MainGameUpdate()
// 2) a)  player controls
// 2) b) update the fan
// 2) c) update the tools that comes out of the fan
void HandlePlayerControls();
void UpdateFan();
void UpdateTools();
void UpdateCoinsAndStars();
void UpdateLasers();
void UpdateDestroyed();


//-------------------
// 2. MAIN_GAME_ENTRY The entry point for a PlayBuffer program
//-------------------
void MainGameEntry(PLAY_IGNORE_COMMAND_LINE)
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);

	Play::CentreAllSpriteOrigins();

	Play::LoadBackground("Data\\Backgrounds\\background.png");

	Play::StartAudioLoop("music"); // will look for the 1st .wav called music in the Data\\Audio directory

	// position at 115/600 with collision radius 50 in pixel, find the first object tiwh agent8 in the name 
	Play::CreateGameObject(TYPE_AGENT8, { 115,600 }, 50, "agent8");


	// create fan and position it at 1140, 503 with zero collison radius and then set the speed.
	// note that the GetGameObject() returns a gameobject that is why we can use the methods .velocity etc
	int id_fan = Play::CreateGameObject(TYPE_FAN, { 1140,503 }, 0, "fan");
	Play::GetGameObject(id_fan).velocity = {0,-3};
	Play::GetGameObject(id_fan).animSpeed = 1.0f;
}


// 3. Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate(float elapsedTime)
{
	// get the elapsedTime passed into the MainGameUpdate and add it to the timer
	// gameState is a global struct
	gameState.timer += elapsedTime;

	Play::DrawBackground();
	HandlePlayerControls();
	UpdateFan();
	UpdateTools();
	UpdateCoinsAndStars();
	UpdateLasers();
	UpdateDestroyed();

	//// draw the sprite name based on the spriteId as a text in the middle of the screen in white
	//// spriteId was set to 0 at the top of this code
	//Play::DrawDebugText
	//( 
	//		{ DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2 }
	//		, Play::GetSpriteName(gameState.spriteId) // returns a char datatype
	//		, Play::cWhite
	//);

	//// draw the sprite image based on spriteID based on where the user mouse is. 
	///*
	//The gameState.timer is used to provide the index of the animation frame.
	//It does not matter if the timer value will exceed the number of frames in any animation as the
	//DrawSprite function will automatically wrap around back to the start of the animation.
	//*/
	//Play::DrawSprite
	//(
	//	gameState.spriteId
	//	, Play::GetMousePos()
	//	, gameState.timer
	//);

	////* when user press the spacebar, increase the spriteId by one to rotate through the sprite.
	//if (Play::KeyPressed(Play::KEY_SPACE)) {
	//	gameState.spriteId++;
	//};

	Play::PresentDrawingBuffer();
	return Play::KeyDown( KEY_ESCAPE );
}

// 4 define the game functions
//4.1 player controls

void HandlePlayerControls() {
	// game controls for Agent8
	// &obj_agent8 is created here as a ref to the agent8 GameObject 
	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	
	if ( Play::KeyDown(Play::KEY_UP)) {
		obj_agent8.velocity = { 0,4 }; // increase the y value by 4
		Play::SetSprite( obj_agent8, "agent8_climb", 0.25f); // change the animation to the climb animation and set the speed to 1/4 of full speed
	} else if (Play::KeyDown(Play::KEY_DOWN)) {
		obj_agent8.acceleration = { 0, -1 }; // set the y axis to accelerate to give impression of falling under gravity
		Play::SetSprite(obj_agent8, "agent8_fall", 0); //change animation to the fall animation and set speed to 0
	}
	else { // when neither is up or down arrow key is pressed i.e. spider is hanging
		Play::SetSprite(obj_agent8, "agent8_hang", 0.02f); // change the animation to the hang animation
		obj_agent8.velocity *= 0.5f; // note that velocity and accelerate are both 2DVector so setting to 1 number means * to both of the x and y axis
		obj_agent8.acceleration = { 0, 0 };
	}

	// ask PlayManager to apply these changes to the objects
	Play::UpdateGameObject(obj_agent8);

	// once the changes are applied, check that the object has left the display area. 
	// if it has, move it back to the previous position
	if ( Play::IsLeavingDisplayArea(obj_agent8)) {
		obj_agent8.pos = obj_agent8.oldPos;
	}

	// then draw the object and the long white web line

	Play::DrawLine(
		{obj_agent8.pos.x, 720},
		obj_agent8.pos,
		Play::cWhite
	);

	// as we want to display the spider to be rotating later, we use DraObjectRotated for agent8 instead of Play::DrawObject
	Play::DrawObjectRotated(obj_agent8);

}

//4.2 update fan
// create a reference to the fan and draw it
void UpdateFan() {
	// create the fan object
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);

	// draw the screw driver
	if (Play::RandomRoll(50) == 50) {
		// Draw the screw driver at the position of the fan with the collision radius as 50 pixel
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		
		GameObject& obj_tool = Play::GetGameObject(id);
		
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6); // randomly set the velocity. setting the x = -8 and y = {-1, 0, 1}*6 which means it can go diagonally up or down or straight across

		// for 50% of the time, do spanner instead
		if (Play::RandomRoll(2) == 1) {
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
		}

		// play the audio 
		Play::PlayAudio("tool");
	}

	// Add the coins to be thrown out by the fan
	// create the coin at the position of the fan and with a collision radius of 40px
	if (Play::RandomRoll(150) ==1) {
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40,"coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = {-3,0};
		obj_coin.rotSpeed = 0.1f;
	}

	// update the object
	Play::UpdateGameObject(obj_fan);

	// check if the fan is leaving the display area
	if (Play::IsLeavingDisplayArea(obj_fan)) {
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity.y *= -1;
	}
	// draw the fan to the screen
	Play::DrawObject(obj_fan);
}
//4.3 update tools
// create a vector to store the tools objects as there will be many of them 
// check collision with agent8
void UpdateTools() {

	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	for (int id : vTools) {
		
		GameObject& obj_tool = Play::GetGameObject(id);

		// check for collision
		if (Play::IsColliding(obj_tool, obj_agent8)) {
			Play::StopAudio("music");
			Play::StopAudio("die");
			obj_agent8.pos = {-100, -100}; //move agent8 off screen
		}

		Play::UpdateGameObject(obj_tool);

		// check if tool is leaving the display area
		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL)) {
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}

		// draw the object
		Play::DrawObjectRotated(obj_tool);

		// when the tools are not longer visible on screen destroy it otherwise clog up the memory
		if (!Play::IsVisible(obj_tool)) {
			Play::DestroyGameObject(id);
		}

	}

}



//4.4 update Coins and stars
/* same as the tools. create a vector to store the coins and loop through the vector
*/
void UpdateCoinsAndStars() {

	GameObject& obj_agent8 = Play::GetGameObjectByType(TYPE_AGENT8);
	std:vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_coin :vCoins) {

		GameObject& obj_coin = Play::GetGameObjectByType(id_coin);

		/* set hasCollided to default false. use this variable to store that a collision has happened instead
		* of destroying the coins immediately after collision.
		* We can't destroy obj_coin immediately because we need to reference it for the Stars outside of the if statement
		*/ 
		bool hasCollided = false; 

		// check if coin collided with spider and create the stars if collided
		if (Play::IsColliding(obj_coin, obj_agent8)) {

			/*create the stars after the coin has collided with the spider
			* create a loop starting from 0.25pi += 0.5pi till 2pi
			* this will produce 4 stars that are diagonally opposite corners from the coin upon collision with spider
			*/
			for (float rad{ 0.25f }; rad < 2.0f; rad += 0.5f) {
				// create a star at 45deg, 135deg, 225deg & 275deg
				int id = Play::CreateGameObject(TYPE_STAR, obj_agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(id);
				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = {0.0f, -0.5f};
				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			hasCollided = true;
			gameState.score += 500;
			Play::PlayAudio("collect");
		}

		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		// check if coin is invisible OR if hasCollided is true then destroy it
		if (!Play::IsVisible(obj_coin) ||  hasCollided) {
			Play::DestroyGameObject(id_coin);
		}

	}

	// 
	std::vector<int> vStars = Play::CollectGameObjectIDsByType(TYPE_STAR);

	for (int id_star : vStars) {
		GameObject& obj_star = Play::GetGameObject(id_star);

		Play::UpdateGameObject(obj_star);
		Play::DrawObjectRotated(obj_star);

		if (!Play::IsVisible(obj_star)) {
			Play::DestroyGameObject(id_star);
		}
	}
}


//4.5 update lasers
/*
*/

void UpdateLasers() {
	std::vector<int> vLasers = Play::CollectGameObjectIDsByType(TYPE_LASER);
	std::vector<int> vTools = Play::CollectGameObjectIDsByType(TYPE_TOOL);
	std::vector<int> vCoins = Play::CollectGameObjectIDsByType(TYPE_COIN);

	for (int id_laser : vLasers) {
		GameObject& obj_laser = Play::GetGameObject(id_laser);
		bool hasCollided = false;

		for (int id_tool : vTools) {

			GameObject& obj_tool = Play::GetGameObject(id_tool);

			if (Play::IsColliding(obj_laser, obj_tool)) {
				hasCollided = true;
				obj_tool.type = TYPE_DESTROYED;
				gameState.score += 100;
			}
		}

		for (int id_coin : vCoins) {

			GameObject& obj_coin = Play::GetGameObject(id_coin);

			if (Play::IsColliding(obj_laser, obj_coin)) {
				hasCollided = true;
				obj_coin.type = TYPE_DESTROYED;
				Play::PlayAudio("error");
				gameState.score -= 300;
			}
		}

		Play::UpdateGameObject(obj_laser);
		Play::DrawObject(obj_laser);

		// check if laser is invisible OR if hasCollided is true then destroy it
		if (!Play::IsVisible(obj_laser) || hasCollided) {
			Play::DestroyGameObject(id_laser);
		}

	}
}

//464 update destroyed
/* 
*/

void UpdateDestroyed() {
	std::vector<int> vDead = Play::CollectGameObjectIDsByType(TYPE_DESTROYED);

	for (int id_dead : vDead) {
		GameObject& obj_dead = Play::GetGameObject(id_dead);
		obj_dead.animSpeed = 0.2f;
		Play::UpdateGameObject(obj_dead);

		if (obj_dead.frame % 2) {
			Play::DrawObjectRotated(obj_dead, (10 - obj_dead.frame) / 10.0f);
		}

		if (!Play::IsVisible(obj_dead) || obj_dead.frame >= 10) {
			Play::DestroyGameObjectsByType(id_dead);
		}
	}
}

// 5. Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

