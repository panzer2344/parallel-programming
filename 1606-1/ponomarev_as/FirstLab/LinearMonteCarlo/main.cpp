#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdlib.h>
#include <Windows.h>

#include <ctime>
#include <stdio.h>

using namespace std;


double function(double x); // function for calculating e.g. linear like this y = x

int main(int argc, char* argv[]) {
	double result = 0.0;

	const int measurementsNumber = 1000; // number of the intervals
	const double rightBorder = 10.0, leftBorder = 0.0;
	double segmentLength = rightBorder - leftBorder;

	double xi; // random point in interval [a, b]
	double SumValue = 0.0; // value of partial sum in process
	double startTime = 0.0, finishTime = 0.0;  // for calculating of the elapsed time


	srand(time(0) | clock()); // needed for getting random value to calculate xi

	/* varriables, which need for printing start time*/
	time_t rawTime;
	struct tm * timeInfo;
	char timeBuffer[80];

	/*init start time*/
	startTime = time(0);

	/*printing nowtime*/
	time(&rawTime);
	timeInfo = localtime(&rawTime);
	strftime(timeBuffer, 80, "Now it's %I:%M%p.", timeInfo);  // formatting datetime
	cout << timeBuffer << endl;

	/* calculating the value of a sum*/
	for (int i = 0; i < measurementsNumber; i++) {
		double ri = (double)rand() / RAND_MAX; // getting a random value in interval [a, b] for calculating xi
		xi = leftBorder + segmentLength * ri;  // that formula  for xi
		cout << "Process #0" << " ri = " << ri << " xi = " << xi << endl;
		SumValue += function(xi);
	}

	/* calculating result*/
	result = segmentLength * SumValue / measurementsNumber;

	/* getting finish time */
	finishTime = time(0);

	cout << "Result = " << result << endl;
	cout << "Elapsed time = " << finishTime - startTime << endl;

	return 0;
}

double function(double x) {
	return x;
}