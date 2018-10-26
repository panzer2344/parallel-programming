#define _CRT_SECURE_NO_WARNINGS
#define GATHER

#include <mpi.h>
#include <iostream>
#include <stdlib.h>
#include <Windows.h>

#include <ctime>
#include <stdio.h>

using namespace std;


double function(double x); // function for calculating e.g. linear like this y = x

int main(int argc, char* argv[]) {
	
	/*init vars*/
	int size, rank;
	double result = 0.0;

	const int measurementsNumber = 1000000; // number of the intervals
	const double rightBorder = 10.0, leftBorder = 0.0;
	double segmentLength = rightBorder - leftBorder;

	int restElementsNumber = 0;// the number of elements in the rest of summing

	double xi = 0.0; // random point in interval [a, b]
	double partialSumValue = 0.0; // value of partial sum in process
	int partSize = 0; // size of part
	double *X = NULL; // all generated random values of x
	double *XPart = NULL; // generated random values of x for each process

	double startTime = 0.0, finishTime = 0.0;  // for calculating of the elapsed time
	MPI_Status status;

	double *receivedPartialSumValues = NULL;

	/* starting parrallel work*/
	MPI_Init(&argc, &argv);

	MPI_Comm_size(MPI_COMM_WORLD, &size);  //getting number of processes
	MPI_Comm_rank(MPI_COMM_WORLD, &rank); // getting number of the current process

	partSize = measurementsNumber / size;
	restElementsNumber = measurementsNumber % partSize;

	XPart = new double[partSize];

	if (rank == 0) {
		srand(time(0) | clock()); // needed for getting random value to calculate xi

		X = new double[measurementsNumber];

		for (int i = 0; i < measurementsNumber; i++) {
			double ri = (double)rand() / RAND_MAX; // getting a random value in interval [a, b] for calculating xi
			X[i] = leftBorder + segmentLength * ri;  // that formula  for xi
		}
	}

	MPI_Scatter(X, partSize, MPI_DOUBLE, XPart, partSize, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	/*===================================================================================================*/
	/*===================================================================================================*/
	/*==============================================Linear Version=======================================*/
	/*===================================================================================================*/
	/*===================================================================================================*/
	if (rank == 0) {
		double SumValue = 0.0; // value of partial sum in process

		/* varriables, which need for printing start time*/
		time_t rawTime;
		struct tm * timeInfo;
		char timeBuffer[80];

		/*printing nowtime*/
		time(&rawTime);
		timeInfo = localtime(&rawTime);
		strftime(timeBuffer, 80, "Now it's %I:%M%p.", timeInfo);  // formatting datetime
		std::cout << timeBuffer << endl;

		/*init start time*/
		startTime = MPI_Wtime();

		/* calculating the value of a sum*/
		for (int i = 0; i < measurementsNumber; i++) {
			//double ri = (double)rand() / RAND_MAX; // getting a random value in interval [a, b] for calculating xi
			//xi = leftBorder + segmentLength * ri;  // that formula  for xi
			//std::cout << "Process #0" << " xi = " << X[i] << endl;
			SumValue += function(X[i]);
		}

		/* calculating result*/
		result = segmentLength * SumValue / measurementsNumber;

		/* getting finish time */
		finishTime = MPI_Wtime();

		std::cout << "Linear version: " << endl;
		std::cout << "Result = " << result << endl;
		std::cout << "Elapsed time = " << finishTime - startTime << endl << endl << endl;

		xi = 0.0;
		result = 0.0;
		startTime = 0.0;
		finishTime = 0.0;

		MPI_Barrier(MPI_COMM_WORLD);
	}
	else {
		MPI_Barrier(MPI_COMM_WORLD);
	}


	/*===================================================================================================*/
	/*===================================================================================================*/
	/*==============================================Parallel Version=====================================*/
	/*===================================================================================================*/
	/*===================================================================================================*/

	if (rank == 0) {
		startTime = MPI_Wtime();
		receivedPartialSumValues = new double[size];
		for (int i = 0; i < size; i++) {
			receivedPartialSumValues[i] = 0;
		}
	}

	/* calculating the value of a partial sum*/
	for (int i = 0; i < partSize; i++) {
		//double ri = (double)rand() / RAND_MAX; // getting a random value in interval [a, b] for calculating xi
		//xi = leftBorder + segmentLength * ri;  // that formula  for xi
											   //cout << "Process #" << rank << " ri = " << ri << " xi = " << xi << endl;
		partialSumValue += function(XPart[i]);
	}

#ifdef GATHER
	MPI_Gather(&partialSumValue, 1, MPI_DOUBLE, receivedPartialSumValues, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
#endif // !GATHER

	if (rank == 0) {

		/*calculating the value of a partial sum for the remaining xi */
		for (int i = measurementsNumber - restElementsNumber; i < measurementsNumber; i++) {
			//double ri = (double)rand() / RAND_MAX;
			//xi = leftBorder + segmentLength * ri;
			xi = X[i];
			std::cout << "Process #" << rank << " xi = " << xi << endl;
			partialSumValue += function(xi);
		}

#ifdef GATHER
		for (int i = 1; i < size; i++) {
			partialSumValue += receivedPartialSumValues[i];
		}
#else
		/*receiving the partial sums from other processes */
		for (int i = 1; i < size; i++) {
			double receivedPartialSumValue = 0.0;
			MPI_Recv(&receivedPartialSumValue, 1, MPI_DOUBLE, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			partialSumValue += receivedPartialSumValue;
		}
#endif // !GATHER

		result = segmentLength * partialSumValue / measurementsNumber; // monte carlo integration formula

		/*getting and printing elapsed time and printing result*/
		finishTime = MPI_Wtime();

		std::cout << "Parallel";
#ifdef GATHER
		std::cout << "(with Gather)";
#endif // GATHER
		std::cout << "version: " << endl;
		std::cout << "Result = " << result << endl;
		std::cout << "Elapsed time = " << finishTime - startTime << endl
			<< "On measurements number = " << measurementsNumber << endl << endl;

		delete receivedPartialSumValues;
	}
#ifndef GATHER
	else {
		MPI_Send(&partialSumValue, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD); // sending partial sum to host process
	}
#endif // !GATHER

	MPI_Finalize();

	return 0;
}

double function(double x) {
	return x;
}