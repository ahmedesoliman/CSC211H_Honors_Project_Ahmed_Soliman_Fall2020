//predict.h
#pragma once

#ifndef PREDICT_H
#define PREDICT_H

#include "app.h"

class Predict : public App {
private:

public:
	Predict();
	~Predict();
	void predictApp(char);
	void displayLetter();
	void load_ASL();

};

#endif // end of PREDICT_H