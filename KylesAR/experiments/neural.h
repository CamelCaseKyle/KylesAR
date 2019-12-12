/*

	http://code-spot.co.za/2009/10/08/15-steps-to-implemented-a-neural-net/

	matrix transposition
		cv::Mat Mt = M.t();
	
	matrix addition
		cv::Mat Ma = M + Mt;
	
	matrix multiplication with a scalar
		cv::Mat Ms = M * 2.0;
	
	ordinary matrix multiplication
		cv::Mat Mm = M * Ms;
	
	Hadamard multiplication(component - wise multiplication)
		cv::Mat Mh;
		cv::multiply(M, Ms, Mh);
	
	Kronecker multiplication(only necessary for between row and column vectors)
		cv::Mat Mk;
		kronecker(M, Ms, Mk);
	
	horizontal matrix concatenation
		cv::Mat Mc, args = { M, Ms };
		cv::hconcat(args, Mc);

*/

#pragma once

#include <opencv2/core.hpp>
#include "winderps.h"

//#define DBG_STACK_TRACE
//#define DBG_LEARN
#define PLOT_GRAPHS

//print a matrix
void printm(cv::Mat &m);

//Kronecker multiplication
void kronecker(cv::Mat &l, cv::Mat &r, cv::Mat &o);

//output matrix to class vector
void outm2classv(cv::Mat &m, cv::Mat &v);
//class vector to output matrix
void classv2outm(cv::Mat &v, cv::Mat &m);

//activate x, store in y
void activate(cv::Mat &x, cv::Mat &y);
//derivitive activate x, store in y
void dxactivate(cv::Mat &x, cv::Mat &y);

//step 5
void feedforward(cv::Mat &input, cv::Mat &weight, cv::Mat &bias, cv::Mat &output, cv::Mat &net);
//init random weights for network
void initWeight(float maxWeight, int width, int height, cv::Mat &m);
//evaluate the error of the network
void evalError(cv::Mat &input, cv::Mat &weight, cv::Mat &output, cv::Mat &clas, cv::Mat &bias);
//dummy backpropagation function
void backprop(cv::Mat &input, cv::Mat &weight, cv::Mat &learn, cv::Mat &bias, cv::Mat &newWeight);


//subset in dataset
class SubSet {
public:
	int training_count;
	cv::Mat inputs,		//rows: training_count cols: input_count
			outputs,	//rows: training_count cols: 1
			classes,	//rows: training_count cols: output_count
			bias;		//rows: training_count cols: 1
};

//dataset loader
class DataSet {
	cv::Mat _weights;
private:
	int input_count;
	int output_count;
	float learnRate = .025f;
	SubSet training_set;
	SubSet validation_set;
	SubSet test_set;
	//load dataset, inputWidth is # columns between concatinated Mats, delim is char between Mat entries
	void load(std::vector<std::string> fileNames, char delim = '\t');
#ifdef PLOT_GRAPHS
	void train(cv::Mat &weight, std::vector<float> &te, std::vector<float> &tc, std::vector<float> &ve, std::vector<float> &vc, std::vector<float> &se, std::vector<float> &sc);
#else
	void train(cv::Mat &weight);
#endif
public:
	//do everything
	void start(std::vector<std::string> fileNames, char delim = '\t');
};
