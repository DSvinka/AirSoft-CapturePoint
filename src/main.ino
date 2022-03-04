#include "GyverButton.h"
#include "GyverTM1637.h"
#include "TimerMs.h"
#include <Arduino.h>

#pragma region Настройка

// Пины Левого Дисплея
#define LEFT_DISPLAY_CLK 3
#define LEFT_DISPLAY_DIO 4

// Пины Правого Дисплея
#define RIGHT_DISPLAY_CLK 5
#define RIGHT_DISPLAY_DIO 6

// Пины Кнопки
#define LEFT_TEAM_BUTTON_PIN 7
#define RIGHT_TEAM_BUTTON_PIN 8

// Пины Лампочки
#define LEFT_TEAM_LED_PIN 9
#define RIGHT_TEAM_LED_PIN 10

// Пины Пищалки
#define SOUND_PIN 11

// Раз во сколько секунд будут выдаваться очки.
const int SECONDS_TO_ADD_SCORE = 5;

// Сколько секунд нужно держать кнопку чтоб захватить точку.
const int SECONDS_ON_BUTTON_TO_CHANGE_TEAM = 5;

// Сколько очков будет выдаваться.
const int ADD_SCORE_COUNT = 1;

// Яркость дисплея команды которая захватила точку. (От 0 до 7)
const int CURRENT_TEAM_DISPLAY_BRIGHTNRSS = 7;
// Яркость дисплея команды которая не захватила точку. (От 0 до 7)
const int OTHER_TEAM_DISPLAY_BRIGHTNRSS = 4;

#pragma endregion

#pragma region Объекты

enum TeamType {
	None = 0,
	LeftTeam = 1,
	RightTeam = 2,
};

enum SoundType {
	Single = 0,
	Multiple = 1,
	Long = 2,
	Error = 3,
};

#pragma endregion

#pragma region Рабочие Переменные

GyverTM1637 leftDisplay(LEFT_DISPLAY_CLK, LEFT_DISPLAY_DIO);
GyverTM1637 rightDisplay(RIGHT_DISPLAY_CLK, RIGHT_DISPLAY_DIO);

GButton leftButton(LEFT_TEAM_BUTTON_PIN);
GButton rightButton(RIGHT_TEAM_BUTTON_PIN);

TimerMs addScoreTimer(SECONDS_TO_ADD_SCORE * 1000, 0, 0);
TimerMs changeTeamButtonTimer(SECONDS_ON_BUTTON_TO_CHANGE_TEAM * 1000, 0, 0);

int leftScore = 0;
int rightScore = 0;

TeamType currentTeam = None;
int currentScore = 0;

#pragma endregion

#pragma region Инициализация

void setup() {
	Serial.begin(9600);

	setupButton(leftButton);
	setupButton(rightButton);

	setupSoundSignal();
	setupLights();

	setupDisplay(leftDisplay);
	setupDisplay(rightDisplay);
	setupTimers();

	serialLog("==== Started ====");
	playSound(Multiple);
}

void setupTimers() {
	serialLog("Setup Timers...");

	addScoreTimer.start();
	changeTeamButtonTimer.stop();
}

void setupLights() {
	serialLog("Setup Lights...");

	pinMode(LEFT_TEAM_LED_PIN, OUTPUT);
	pinMode(RIGHT_TEAM_LED_PIN, OUTPUT);
}

void setupButton(GButton button) {
	serialLog("Setup Button...");
	button.setDebounce(50);        // настройка антидребезга (по умолчанию 80 мс)
	button.setTimeout(500);        // настройка таймаута на удержание (по умолчанию 500 мс)
	button.setClickTimeout(600);   // настройка таймаута между кликами (по умолчанию 300 мс)
	button.setType(HIGH_PULL);
	button.setDirection(NORM_OPEN);
}

void setupSoundSignal() {
	serialLog("Setup SoundSignal...");
	pinMode(SOUND_PIN, OUTPUT);
}

void setupDisplay(GyverTM1637 display) {
	serialLog("Setup Display...");
	display.clear();
	display.displayInt(0);
	display.brightness(OTHER_TEAM_DISPLAY_BRIGHTNRSS);
}

#pragma endregion

#pragma region Функционал

void loop() {
	leftButton.tick();
	rightButton.tick();

	TeamType buttonTeam = checkCurrentTeam();
	if (currentTeam == buttonTeam && changeTeamButtonTimer.active()) {
		changeTeamButtonTimer.stop();
	}
	else if (currentTeam != buttonTeam && !changeTeamButtonTimer.active()) {
		changeTeamButtonTimer.start();
	}

	if (changeTeamButtonTimer.tick()) {
		changeTeamButtonTimerCallBack();
	}

	if (addScoreTimer.tick()) {
		scoreTimerCallback();
	}
}

TeamType checkCurrentTeam() {
	if (leftButton.isHold()) {
		return LeftTeam;
	}
	else if (rightButton.isHold()) {
		return RightTeam;
	}
	return currentTeam;
}

int addScore(TeamType teamType) {
	switch (teamType) {
		case LeftTeam:
			leftScore += 1;
			return leftScore;

		case RightTeam:
			rightScore += 1;
			return rightScore;
		
		default:
			serialLog("AddScore ERROR: Team " + String(teamType) + " Not Provider");
			playSound(Error);
			return 0;
	}
}

void updateDisplay(TeamType teamType, int score) {
	switch (teamType) {
		case LeftTeam:
			leftDisplay.displayInt(score);
			leftDisplay.brightness(CURRENT_TEAM_DISPLAY_BRIGHTNRSS);
			rightDisplay.brightness(OTHER_TEAM_DISPLAY_BRIGHTNRSS);
			break;

		case RightTeam:
			rightDisplay.displayInt(score);
			rightDisplay.brightness(CURRENT_TEAM_DISPLAY_BRIGHTNRSS);
			leftDisplay.brightness(OTHER_TEAM_DISPLAY_BRIGHTNRSS);
			break;

		default:
			serialLog("UpdateDisplay ERROR: Team " + String(teamType) + " Not Provider");
			playSound(Error);
	}
}

void updateLed(TeamType teamType) {
	switch (teamType) {
		case LeftTeam:
			digitalWrite(LEFT_TEAM_LED_PIN, HIGH);
			digitalWrite(RIGHT_TEAM_LED_PIN, LOW);
			break;

		case RightTeam:
			digitalWrite(LEFT_TEAM_LED_PIN, LOW);
			digitalWrite(RIGHT_TEAM_LED_PIN, HIGH);
			break;

		default:
			serialLog("UpdateLed ERROR: Team " + String(teamType) + " Not Provider");
			playSound(Error);
	}
}


#pragma endregion

#pragma region Callback для таймеров

void scoreTimerCallback() {
	if (currentTeam == None) {
		return;
	}

	currentScore = addScore(currentTeam);
	updateDisplay(currentTeam, currentScore);
	updateLed(currentTeam);
	playSound(Single);

	serialLog("Score Added To Team " + String(currentTeam) + ": Total Score = " + String(currentScore));
}

void changeTeamButtonTimerCallBack() {
	TeamType buttonTeam = checkCurrentTeam();
	if (currentTeam == buttonTeam) {
		return;
	}

	serialLog("Team Changed: From " + String(currentTeam) + " To " + String(buttonTeam));
	playSound(Multiple);
	currentTeam = buttonTeam;
}

#pragma endregion

#pragma region Звуковые Утилиты

void playSound(SoundType soundType) {
	switch (soundType) {
		case Single:
			playSignalSound(500);
			break;

		case Multiple:
			for (int i = 0; i < 3; i++) {
				playSignalSound(400);
				delay(400);
			}
			break;

		case Long:
			playSignalSound(2000);
			break;

		case Error:
			for (int i = 0; i < 2; i++) {
				for (int i = 0; i < 3; i++) {
					playSignalSound(500);
					delay(500);
				}
				playSignalSound(2000);
			}
			break;

		default:
			serialLog("PlaySound ERROR: SoundType " + String(soundType) + " Not Provider");
			playSound(Error);
			break;
	}
}

void playSignalSound(long soundLength) {
	digitalWrite(SOUND_PIN, HIGH);
	delay(soundLength);
	digitalWrite(SOUND_PIN, LOW);
}

#pragma endregion

#pragma region Отладочные Утилиты

void serialLog(String message) {
	if (Serial) {
		Serial.println(message);
	}
}

#pragma endregion