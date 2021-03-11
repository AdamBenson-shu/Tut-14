#include <assert.h>
#include <sstream>

#include "SFML/Graphics.hpp"
#include "SFML/Audio.hpp"
#include "Utils.h"
#include "MyDB.h"

using namespace sf;
using namespace std;

//*************************************************
//game constants
namespace GC {
	const char ESCAPE_KEY = 27;
	const char BACKSPACE_KEY = 8;
	const char ENTER_KEY = 13;
	const char ZERO_KEY = 48;
	const vector<IntRect> SPR_DIMS = {	//image coordinates for the fruit
		{0, 0,   94, 94},
		{0, 94,  94, 94},
		{0, 188, 94, 94},
		{0, 282, 94, 94},
		{0, 376, 94, 94},
		{0, 470, 94, 94}
	};
	const vector<IntRect> SPR_DIMS_SPIN = { //image coordinates when the fruit spin
		{94, 0,   94, 94},
		{94, 94,  94, 94},
		{94, 188, 94, 94},
		{94, 282, 94, 94},
		{94, 376, 94, 94},
		{94, 470, 94, 94}
	};
	const vector<string> SPR_NAMES = { //fruit names
		"orange",
		"seven",
		"bar",
		"pear", 
		"banana",
		"cherry"
	};
	const IntRect HOLD_DIMS = {188,0,78,29}; //a "hold" image
	const vector<int> CASH_PRIZES = { 15, 20, 30, 50, 100, 250 };	//how much a win is worth for each fruit
	const int NUDGE_COST = 5;		//cost to nudge
	const int HOLD_COST = 6;		//cost to hold
	const int PLAY_COST = 5;		//cost to play
	const int START_CASH = 200;		//starting pot
	const int MAX_NUDGEHOLD = 10;	//how many times you can hold and nudge before you have to spin again
	const float SPIN_TIME = 2.f;	//how long a full spin is meant to last	
	const int MAX_NAME = 8;			//max characters in player name
	const int MAX_HIGHSCORES = 10;	//only save and show 10 of them
}

//*************************************************
//handles the 5 reels of the slot machine, spinning them around
//instructions for the slots
struct Slots
{
	//there are five reels
	struct Data {
		int result=0;		//what it current shows, matches fruit order on sprite sheet
		float spinTime=0;	//how long this reel has left to spin
		bool hold = false;	//is this reel meant to be holding
	};
	vector<Data> reels;

	Texture texIcons;			//all fruit sprites on one texture
	bool spinning = false;		//are we spinning right now?
	float spinTimer = 0;		//how long to spin
	bool winningRound = false;	//did we just win a prize - all fruit same on one line
	int nudgeHoldCtr = GC::MAX_NUDGEHOLD;	//how many times have we left to nudge or hold?

	//set everything up
	void Init();
	//setup the reels teh first time
	void Reset();
	//spin one or more reels
	void Spin();
	//render and update the reels
	void Render(RenderWindow& window, float elapsed, Font& font);
	void Update(RenderWindow& window, float elapsed);
	//how much did we win on the last spin?
	int GetWinnings();
	//nudge a specific real (0-5), makes just that reel spin
	void Nudge(int reel);
	//hold a specific reel (0-5), makes all reels spin other than this one
	void Hold(int reel);
	//show what a line of fruit is worth
	void RenderInstructions(RenderWindow& window, Font& font);
	//can we nudge or hold anymore of have we ran out of goes and need to spin?
	bool CanNudgeAndHold() {
		return nudgeHoldCtr > 0;
	}
};

void Slots::Nudge(int reel)
{
	assert(nudgeHoldCtr > 0);
	--nudgeHoldCtr;
	winningRound = false;
	spinning = true;
	spinTimer = GetClock() + (GC::SPIN_TIME/5); //time to spin just one reel
	//ensure all reels will not spin
	for (size_t i = 0; i < reels.size(); ++i)
		reels[i].spinTime = 0; 
	//let the one we are interested in spin
	reels[reel].spinTime = GetClock() + (GC::SPIN_TIME / 5);
	reels[reel].hold = false;
}

void Slots::Hold(int reel)
{
	assert(nudgeHoldCtr > 0);
	--nudgeHoldCtr;
	winningRound = false;
	spinning = true;
	spinTimer = GetClock() + (GC::SPIN_TIME * 0.8f); //time to spin 4 of the reels
	for (int i = 0; i < 5; ++i)
		if (i != reel)
		{
			//if we aren't holding it, set up to sping
			reels[i].spinTime = GetClock() + (GC::SPIN_TIME / (5 - i));
			reels[i].hold = false;
		}
		else
			//hold just this one
			reels[i].hold = true;
}

int Slots::GetWinnings()
{
	//don't call this unless you know we won or it will assert
	assert(winningRound && reels[0].result>=0 && reels[0].result<=5);
	return GC::CASH_PRIZES[reels[0].result]; //figure out what a line is worth
}

void Slots::Init()
{
	if (!texIcons.loadFromFile("data/slots.png"))
		assert(false);
	Reset();
}

void Slots::Update(RenderWindow& window, float elapsed)
{
	if (spinning)
	{
		//as each reel finishes spinning, set it to a random fruit
		for (size_t i = 0; i < reels.size(); ++i)
			if (reels[i].spinTime < GetClock() && reels[i].spinTime>0)
			{
				reels[i].spinTime = 0;
				reels[i].result = Rnd::GetRange(0, 5);
			}
		//have all reels stopped yet?
		if (spinTimer < GetClock())
		{
			spinning = false;
			int win = reels[0].result, cnt = 1;
			for (size_t i = 1; i < reels.size(); ++i)
			{
				reels[i].hold = false;		//reset each reel
				if (reels[i].result == win)	//keep a tally of matching fruit, maybe we won?
					++cnt;
			}
			winningRound = (cnt == (int)reels.size()) ? true : false; //check if we won, it's a one line if statement
		}
	}
}

void Slots::RenderInstructions(RenderWindow& window, Font& font)
{
	//print out all the fruit and what they are worth
	Sprite spr(texIcons);
	Vector2f off{ 10, 10 };
	for (size_t i = 0; i < 6; ++i)
	{
		spr.setTextureRect(GC::SPR_DIMS[i]);
		spr.setPosition(off);
		spr.setScale(0.3f, 0.3f);
		window.draw(spr);

		stringstream ss;
		ss << GC::SPR_NAMES[i] << " $" << GC::CASH_PRIZES[i];
		Text txt(ss.str(), font, 20);
		txt.setPosition(off.x + spr.getGlobalBounds().width * 1.1f, off.y);
		window.draw(txt);

		off.y += spr.getGlobalBounds().height * 1.1f;
	}
	//keep a tally of how many nudges/holds they've had this spin
	stringstream ss;
	ss << "Nudges and holds left: " << nudgeHoldCtr;
	Text txt(ss.str(), font, 20);
	txt.setPosition(off);
	window.draw(txt);
}

void Slots::Render(RenderWindow& window, float elapsed, Font& font)
{
	RenderInstructions(window, font);
	Vector2f off{ window.getSize().x * 0.3f, window.getSize().y * 0.3f };
	Sprite spr(texIcons);
	Text txt("1", font, 30);
	//render each of the 5 reels
	for (size_t i = 0; i < reels.size(); ++i)
	{
		spr.setPosition(off);
		//is is spinning or steady?
		if (spinning && reels[i].spinTime > GetClock())
			spr.setTextureRect(GC::SPR_DIMS_SPIN[reels[i].result]);
		else
			spr.setTextureRect(GC::SPR_DIMS[reels[i].result]);
		window.draw(spr);

		//is this reel on hold?
		if (spinning && reels[i].hold)
		{
			Sprite spr2(texIcons, GC::HOLD_DIMS);
			spr2.setPosition(off.x, off.y + GC::HOLD_DIMS.height*1.1f);
			window.draw(spr2);
		}
		//each reel has a number so we can nudge/hold it
		txt.setString(std::to_string(i + 1));
		txt.setPosition(off.x + spr.getGlobalBounds().width / 2.f - txt.getGlobalBounds().width/2.f, off.y + spr.getGlobalBounds().height*1.1f);
		window.draw(txt);
		off.x += spr.getGlobalBounds().width * 1.1f;
	}
}

void Slots::Reset()
{
	reels = { {0,0,false},{0,0,false},{0,0,false},{0,0,false},{0,0,false} };
}

void Slots::Spin()
{
	//get everything ready for a new spin
	nudgeHoldCtr = GC::MAX_NUDGEHOLD;	//reset the nudge/hold counter
	winningRound = false;
	spinning = true;
	spinTimer = GetClock() + GC::SPIN_TIME;
	//spin every reel
	for (int i = 0; i < 5; ++i)
	{
		reels[i].hold = false;
		reels[i].spinTime = GetClock() + (GC::SPIN_TIME / (5 - i));
	}
}

//*************************************************
//a slot machine game - take their money, get their name, record how much they win or lose
struct Game
{
	sf::Font font;	//one font for the game
	MyDB myDB;		//store the high score data
	Slots slots;	//spin those reels
	enum class Mode { 
		READY,			//waiting to see if you want to spin
		SPINNING,		//away we go
		RESULT,			//show you the result of a spin
		ENTER_NAME,		//wait for you to enter your name
		HIGH_SCORES,	//show you the high scores
		NUDGE,			//you want to nudge one reel
		HOLD			//you want to hold one reel	
	};
	Mode mode = Mode::READY;

	int cash = 0;						//money in your pot
	string name;						//who are you

	sf::Music music;
	sf::Sound sfxWin, sfxLose, sfxSpin;
	sf::SoundBuffer bufWin, bufLose, bufSpin;

	//set everything up at the start
	void Initialise(RenderWindow& window);
	//once at the end, make sure things are shut down, save the database
	void Release();
	//standard update and render
	void Update(RenderWindow& window, float elapsed, char key, bool keyPress, int& nudge);
	void Render(RenderWindow& window, float elapsed);

	//specialised versions of update
	void UpdateReady(RenderWindow& window, float elapsed, char key, bool keyPress);
	void UpdateSpin(RenderWindow& window, float elapsed, char key, bool keyPress);
	void UpdateResult(RenderWindow& window, float elapsed, char key, bool keyPress);
	void UpdateHoldNudge(RenderWindow& window, float elapsed, char key, bool keyPress);
	void UpdateEnterName(RenderWindow& window, float elapsed, char key, bool keyPress, int nudge);
	void UpdateHighscores(RenderWindow& window, float elapsed, char key, bool keyPress);

	//same again for redering
	void RenderReady(RenderWindow& window, float elapsed);
	void RenderResult(RenderWindow& window, float elapsed);
	void RenderHighscores(RenderWindow& window, float elapsed);
	void RenderName(RenderWindow& window, float elapsed);
	void RenderNudgeHold(RenderWindow& window, float elapsed);
};

void Game::Initialise(RenderWindow& window)
{
	//check the database is setup
	bool doesExist;
	myDB.Init("data/player.db", doesExist);
	if (!doesExist)
	{
		myDB.ExecQuery("CREATE TABLE HIGHSCORES(" \
			"ID				 INTEGER PRIMARY KEY autoincrement,"\
			"NAME			TEXT	NOT NULL,"\
			"SCORE			INT		NOT NULL");
		myDB.ExecQuery("CREATE TABLE PLAYS(" \
			"ID				INTEGER PRIMARY KEY autoincrement,"\
			"HIGHSCORE_ID	INT		NOT NULL,"\
			"TOTAL_PLAYS	INT		NOT NULL,"\
			"TOTAL_NUDGES	INT		NOT NULL");
	}
	slots.Init();
	if (!font.loadFromFile("data/fonts/comic.ttf"))
		assert(false);
	Rnd::Seed();	//see the random numbers to time so it's always different
	cash = GC::START_CASH;

	//play some music permanently
	if (!music.openFromFile("data/music_loop.wav"))
		assert(false);
	music.play();
	music.setLoop(true);
	music.setVolume(50);

	//prepare some sound effects
	if (!bufWin.loadFromFile("data/win.wav"))
		assert(false);
	sfxWin.setBuffer(bufWin);
	if (!bufLose.loadFromFile("data/lose.wav"))
		assert(false);
	sfxLose.setBuffer(bufLose);
	if (!bufSpin.loadFromFile("data/spin.wav"))
		assert(false);
	sfxSpin.setBuffer(bufSpin);
	sfxSpin.setLoop(true);
	sfxSpin.setVolume(15);
}

void Game::Release()
{
	myDB.SaveToDisk();
	myDB.Close();
}

void Game::Update(RenderWindow& window, float elapsed, char key, bool keyPress, int& nudge)
{
	slots.Update(window, elapsed);
	switch(mode)
	{
	case Mode::READY:
		UpdateReady(window, elapsed, key, keyPress);
		break;
	case Mode::SPINNING:
		UpdateSpin(window, elapsed, key, keyPress);
		break;
	case Mode::RESULT:
		UpdateResult(window, elapsed, key, keyPress);
		break;
	case Mode::NUDGE:
	case Mode::HOLD:
		UpdateHoldNudge(window, elapsed, key, keyPress);
		nudge++;
		break;
	case Mode::ENTER_NAME:
		UpdateEnterName(window, elapsed, key, keyPress, nudge);
		break;
	case Mode::HIGH_SCORES:
		UpdateHighscores(window, elapsed, key, keyPress);
		break;
	}
}

void Game::UpdateHighscores(RenderWindow& window, float elapsed, char key, bool keyPress)
{
	//quit
	if (Keyboard::isKeyPressed(Keyboard::Escape))
		window.close();
	//let's have another go, start over
	if (keyPress && Keyboard::isKeyPressed(Keyboard::Space))
	{
		mode = Mode::READY;
		cash = GC::START_CASH;
	}
}

void Game::UpdateEnterName(RenderWindow& window, float elapsed, char key, bool keyPress, int nudge)
{
	if (keyPress)
	{
		if (key == GC::ENTER_KEY && name.length()>1)//they've finished typing
		{
			stringstream ss;
			ss << "SELECT ID, NAME, SCORE FROM HIGHSCORES ORDER BY SCORE ASC LIMIT " << GC::MAX_HIGHSCORES;
			myDB.ExecQuery(ss.str());
			//is this person already in there?
			size_t idx = 0;
			while (idx < myDB.results.size() && myDB.GetStr(idx, "NAME") != name)
			{
				++idx;
			}
			int pot = cash - GC::START_CASH;
			if (idx < myDB.results.size())
			{
				//udpate the existing record
				int winnings = myDB.GetInt(idx, "SCORE");
				int highscoreID = myDB.GetInt(idx, "ID");
				//int onudge = myDB.GetInt(idx, "NUDGETOTAL");
				winnings += pot;
				//tnudge += onudge;
				ss.str("");
				ss << "UPDATE HIGHSCORES SET SCORE = " << winnings << " WHERE NAME='" + name + "'";
				myDB.ExecQuery(ss.str());

				ss << "UPDATE plays set total_play = total_plays + 1, total_nudges = " << nudge << " WHERE highscore_id = " + highscoreID;
				myDB.ExecQuery(ss.str());
			}
				myDB.ExecQuery(ss.str());
			 if (myDB.results.size() < GC::MAX_HIGHSCORES)
			{
				//just add it, we don't have enough
				ss.str("");
				ss << "INSERT INTO HIGHSCORES (NAME, SCORE) VALUES ('" << name << "'," << pot << ")";
				myDB.ExecQuery(ss.str());

				ss << "SELECT ID FROM HIGHSCORES order by ID" << GC::MAX_HIGHSCORES;
				myDB.ExecQuery(ss.str());
				//is this person already in there?
				size_t idx = 0;
				while (idx < myDB.results.size() && myDB.GetStr(idx, "NAME") != name)
				{
					++idx;
				}
				int highscoreID = myDB.GetInt(idx, "ID");
				ss << "INSERT INTO plays (highscore_id, total_plays, total_plays) VALUES (" << highscoreID << ",0,0)";
				myDB.ExecQuery(ss.str());

			}
			else if (pot > myDB.GetInt(myDB.results.size() - 1, "SCORE"))
			{
				//replace the lowest score
				ss.str("");
				ss << "DELETE FROM HIGHSCORES WHERE ID=" << myDB.GetInt(myDB.results.size() - 1, "ID");
				myDB.ExecQuery(ss.str());
				ss.str("");
				ss << "INSERT INTO HIGHSCORES (NAME, SCORE) VALUES ('" << name << "'," << pot;
				myDB.ExecQuery(ss.str());
			}

			mode = Mode::HIGH_SCORES;
		}
		else if ((key == GC::BACKSPACE_KEY) && name.length() > 0)
			name = name.substr(0, name.length() - 1);  //delete a character
		else if (isalpha(key) && name.length() < GC::MAX_NAME)
			name += key;	//add a character
	}
}


void Game::UpdateHoldNudge(RenderWindow& window, float elapsed, char key, bool keyPress)
{
	size_t reel = key - GC::ZERO_KEY; //convert the key press to a number
	if (reel >= 1 && reel <= GC::SPR_NAMES.size() && cash >= GC::NUDGE_COST) //check it's a good number and we have money
	{
		--reel;//turn the key press into an index into the reel array
		if (mode == Mode::NUDGE)
		{
			cash -= GC::NUDGE_COST;
			slots.Nudge(reel);
		}
		else
		{
			cash -= GC::HOLD_COST;
			slots.Hold(reel);
		}
		mode = Mode::SPINNING;
		sfxSpin.play();
	}
}

void Game::UpdateResult(RenderWindow& window, float elapsed, char key, bool keyPress)
{
	if (Keyboard::isKeyPressed(Keyboard::Space) && cash > GC::PLAY_COST)
		mode = Mode::READY; //start again
	if (Keyboard::isKeyPressed(Keyboard::Escape))
	{
		mode = Mode::ENTER_NAME; //check who is playing
		name.clear();
	}
	if (!slots.winningRound && slots.CanNudgeAndHold() && cash > GC::HOLD_COST && cash > GC::NUDGE_COST)
	{	//do they want to nudge/hold and can they afford it
		if (key == 'n')
			mode = Mode::NUDGE;
		if (key == 'h')
			mode = Mode::HOLD;
	}
}

void Game::UpdateReady(RenderWindow& window, float elapsed, char key, bool keyPress)
{
	if (Keyboard::isKeyPressed(Keyboard::Space))
	{
		//let's play
		slots.Spin();
		cash -= GC::PLAY_COST;
		mode = Mode::SPINNING;
		sfxSpin.play();
	}
	else if (Keyboard::isKeyPressed(Keyboard::Escape))
	{
		mode = Mode::HIGH_SCORES;	
	}
}

void Game::UpdateSpin(RenderWindow& window, float elapsed, char key, bool keyPress)
{
	if (!slots.spinning)
	{
		sfxSpin.stop();
		if (slots.winningRound)
		{
			//we won something!!
			cash += slots.GetWinnings();
			sfxWin.play();
		}
		else
			sfxLose.play();
		mode = Mode::RESULT;
	}
}

void Game::Render(RenderWindow& window, float elapsed)
{
	//title
	sf::Text txt("Super Slots!!", font, 30);
	txt.setPosition(window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y*0.05f);
	window.draw(txt);

	Vector2f pos;
	switch(mode)
	{
	case Mode::READY:
		RenderReady(window, elapsed);
		break;
	case Mode::SPINNING:
		slots.Render(window, elapsed, font);
		break;
	case Mode::RESULT:
		RenderResult(window, elapsed);
		break;
	case Mode::NUDGE:
	case Mode::HOLD:
		RenderNudgeHold(window, elapsed);
		break;
	case Mode::ENTER_NAME:
		RenderName(window, elapsed);
		break;
	case Mode::HIGH_SCORES:
		RenderHighscores(window, elapsed);
		break;
	}

	//the pot
	stringstream ss;
	ss << "Bank $" << cash;
	txt.setString(ss.str());
	pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, pos.y = window.getSize().y * 0.7f };
	txt.setPosition(pos);
	window.draw(txt);
}

void Game::RenderNudgeHold(RenderWindow& window, float elapsed)
{
	slots.Render(window, elapsed, font);
	Text txt("Press <1> <2> <3> <4> <5>.", font);
	Vector2f pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.6f };
	txt.setPosition(pos);
	window.draw(txt);
}

void Game::RenderReady(RenderWindow& window, float elapsed)
{
	slots.Render(window, elapsed, font);
	stringstream ss;
	ss << "$" << GC::PLAY_COST << " to play. Press <space> to spin.";
	Text txt(ss.str(), font);
	Vector2f pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.6f };
	txt.setPosition(pos);
	window.draw(txt);
}

void Game::RenderResult(RenderWindow& window, float elapsed)
{
	slots.Render(window, elapsed, font);
	//win lose message
	stringstream ss;
	if (slots.winningRound)
		ss << "You won $" << slots.GetWinnings() << " ";
	else
		ss << "You lose. ";
	if (cash > 0)
		ss << "Press <space> to spin. ";
	ss << "Press <ESC> to quit.";
	Text txt(ss.str(), font);
	Vector2f pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.6f };
	txt.setPosition(pos);
	window.draw(txt);
	//can they save it with a nudge/hold?
	ss.str("");
	ss << "$" << GC::PLAY_COST << " to play. ";
	if (!slots.winningRound && slots.CanNudgeAndHold())
		ss << "Press <n> to nudge a reel $" << GC::NUDGE_COST
		<< ", press <h> to hold a reel $" << GC::HOLD_COST << ".";
	txt.setString(ss.str());
	pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y*0.65f };
	txt.setPosition(pos);
	window.draw(txt);
}

void Game::RenderName(RenderWindow& window, float elapsed)
{
	Text txt("Enter your name", font);
	Vector2f pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.2f };
	txt.setPosition(pos);
	window.draw(txt);

	//show name with a flashing cursor
	stringstream ss;
	ss << name;
	if ((int)GetClock() % 2)
		ss << '_';
	txt.setString(ss.str());
	pos = { window.getSize().x * 0.4f, window.getSize().y * 0.4f };
	txt.setPosition(pos);
	window.draw(txt);

	//instructions
	txt.setString("Undo a character <backspace>, when finished <enter>, eight characters max.");
	pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.6f };
	txt.setPosition(pos);
	window.draw(txt);
}

void Game::RenderHighscores(RenderWindow& window, float elapsed)
{
	Text txt("Highscores", font);
	Vector2f pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.1f };
	txt.setPosition(pos);
	window.draw(txt);

	//get an ordered copy of the score data
	stringstream ss;
	ss << "SELECT NAME, SCORE FROM HIGHSCORES ORDER BY SCORE DESC LIMIT " << GC::MAX_HIGHSCORES;
	myDB.ExecQuery(ss.str());
	pos = { window.getSize().x * 0.3f, window.getSize().y * 0.2f };
	for (size_t i = 0; i < GC::MAX_HIGHSCORES; ++i)
	{
		//print out each line
		ss.str("");
		ss << i + 1 << ".";
		txt.setString(ss.str());
		txt.setPosition(pos);
		window.draw(txt);

		ss.str("");
		if (myDB.results.size() > i)
			ss << myDB.GetStr(i, "NAME");
		else
			ss << "???";
		txt.setString(ss.str());
		txt.setPosition(window.getSize().x * 0.5f, pos.y);
		window.draw(txt);

		ss.str("");
		if (myDB.results.size() > i)
			ss << myDB.GetStr(i, "SCORE");
		else
			ss << "???";
		txt.setString(ss.str());
		txt.setPosition(window.getSize().x * 0.7f, pos.y);
		window.draw(txt);

		pos.y += txt.getGlobalBounds().height * 1.2f;
	}
	//instructions
	txt.setString("Press <ESC> to quit, <space> to keep betting.");
	pos = { window.getSize().x / 2.f - txt.getGlobalBounds().width / 2.f, window.getSize().y * 0.8f };
	txt.setPosition(pos);
	window.draw(txt);
}


//*************************************************
//entry point
int main()
{
	// Create the main window
	int nudge;
	nudge = 0;
	RenderWindow window( VideoMode(1200, 800), "Slots!");

	Game game;
	game.Initialise(window);

	Clock clock;
	// Start the game loop 
	while (window.isOpen())
	{
		bool fire = false, keyPress = false;
		char key = 0;
		// Process events
		Event event;
		while (window.pollEvent(event))
		{
			if (event.type == Event::KeyPressed)
			{
				keyPress = true;
			}
			else if (event.type == Event::TextEntered)
			{
				if (event.text.unicode < 128)
					key = static_cast<char>(event.text.unicode);
			}
			else if (event.type == Event::Closed)
			{
				window.close();
			}
		} 

		// Clear screen
		window.clear();

		float elapsed = clock.getElapsedTime().asSeconds();
		clock.restart();
		
		game.Update(window, elapsed, key, keyPress, nudge);
		game.Render(window, elapsed);
		AddSecsToClock(elapsed);
		
		// Update the window
		window.display();
	}

	game.Release();
	return EXIT_SUCCESS;
}
