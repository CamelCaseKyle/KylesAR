#include "neural.h"


///////////////////////////////// utilities /////////////////////////////////

//random number from l to h
float rndRange(float l, float h) {
	float lh = h - l;
	return float(rand()) / float(RAND_MAX) * lh + l;
}
//random function for weights
float random(float x) {
	return rndRange(-x, x);
}

//print a matrix of floats
void printm(cv::Mat &m) {
	int nRows = m.rows;
	int nCols = m.cols;
	int j;
	double *p;
	if (m.isContinuous()) {
		nRows *= nCols;
		int jj;
		p = m.ptr<double>();
		for (j = 0; j < nRows; ++j) {
			jj = j % nCols;
			printf("%f ", p[j]);
			if (jj == nCols - 1) {
				printf("\n");
			}
		}
	} else {
		int i;
		for (i = 0; i < nRows; ++i) {
			p = m.ptr<double>(i);
			for (j = 0; j < nCols; ++j) {
				printf("%f ", p[j]);
			}
			printf("\n");
		}
	}
}

//Kronecker multiplication
void kronecker(cv::Mat &l, cv::Mat &r, cv::Mat &o) {
#ifdef DBG_STACK_TRACE
	printf("kronecker\n");
#endif
	o = cv::Mat::zeros(l.rows, r.cols, CV_32FC1);
	for (int i = 0; i < l.rows; i++) {
		for (int j = 0; j < r.cols; j++) {
			o.at<float>(i, j) = l.at<float>(i) * r.at<float>(j);
		}
	}
}

//flattens matrix into vector
void outm2classv(cv::Mat &m, cv::Mat &v) {
#ifdef DBG_STACK_TRACE
	printf("outm2classv\n");
#endif
	v = cv::Mat::zeros(m.rows, 1, CV_32FC1);
	int nRows = m.rows;
	int nCols = m.cols;
	int j;
	float *p, *q = v.ptr<float>();
	float val = 0.f;
	if (m.isContinuous()) {
		nCols *= nRows;
		int i, jj;
		float val = 0.f;
		p = m.ptr<float>();
		for (j = 0; j < nCols; ++j) {
			jj = j % nRows;
			val += p[j] * float(jj);
			if (jj == nRows - 1) {
				i = int(float(j) / float(nRows));
				q[i] = val;
				val = 0.f;
			}
		}
	} else {
		int i;
		for (i = 0; i < nRows; ++i) {
			val = 0.f;
			p = m.ptr<float>(i);
			for (j = 0; j < nCols; ++j) {
				val += p[j] * float(j);
			}
			q[i] = val;
		}
	}
}
//squishes vector into matrix
void classv2outm(cv::Mat &v, cv::Mat &m) {
#ifdef DBG_STACK_TRACE
	printf("classv2outm\n");
#endif
	//count highest val in v
	float *q = v.ptr<float>();
	int i, highestVal = 0.f;
	for (i = 0; i < v.cols; ++i) {
		if (int(q[i]) > highestVal) {
			highestVal = q[i];
		}
	}
	//new matrix was probably this size? uh oh
	m = cv::Mat::zeros(v.cols, highestVal, CV_32FC1);
	for (i = 0; i < v.cols; ++i) {
		m.at<float>(i, q[i]) = 1.f;
	}
}

//////////////////// activation functions and derivatives ////////////////////
float activeFuncTanh(float x) {
	float tanhx = tanh(x);
	return (tanhx + 1.f) / 2.f;
}
float dxActiveFuncTanh(float x) {
	float tanhx = tanh(x);
	return (1.f - tanhx * tanhx) / 2.f;
}

float activeFuncReLU(float x) {
	return max(x*.01f, x);
}
float dxActiveFuncReLU(float x) {
	return (x < 0.f)? .01f: 1.f;
}


//apply element wize operation func to members of x, store in y
void elementFunc(cv::Mat &x, cv::Mat &y, float(*func)(float x)) {
	y = cv::Mat::zeros(x.size(), x.type());
	int nRows = x.rows;
	int nCols = x.cols;
	int j;
	float *p, *q;
	if (x.isContinuous()) {
		nCols *= nRows;
		int jj;
		float *q;
		p = x.ptr<float>();
		q = y.ptr<float>();
		for (j = 0; j < nCols; ++j) {
			q[j] = func(p[j]);
		}
	} else {
		int i;
		for (i = 0; i < nRows; ++i) {
			p = x.ptr<float>(i);
			q = y.ptr<float>(i);
			for (j = 0; j < nCols; ++j) {
				q[j] = func(p[j]);
			}
		}
	}
}
//apply element wize operation func with args x, and y, store result in z
void operatorFunc(cv::Mat &x, cv::Mat &y, cv::Mat &z, float(*func)(float x, float y)) {
	z = cv::Mat::zeros(x.size(), x.type());
	int nRows = x.rows;
	int nCols = x.cols;
	int j;
	float *p, *q, *r;
	if (x.isContinuous()) {
		nCols *= nRows;
		int jj;
		float *q;
		p = x.ptr<float>();
		q = y.ptr<float>();
		r = z.ptr<float>();
		for (j = 0; j < nCols; ++j) {
			r[j] = func(p[j], q[j]);
		}
	} else {
		int i;
		for (i = 0; i < nRows; ++i) {
			p = x.ptr<float>(i);
			q = y.ptr<float>(i);
			r = z.ptr<float>(i);
			for (j = 0; j < nCols; ++j) {
				r[j] = func(p[j], q[j]);
			}
		}
	}
}

//activate x, store in y
void activate(cv::Mat &x, cv::Mat &y) {
#ifdef DBG_STACK_TRACE
	printf("activate\n");
#endif
	elementFunc(x, y, &activeFuncReLU);
}
//derivitive activate x, store in y
void dxactivate(cv::Mat &x, cv::Mat &y) {
#ifdef DBG_STACK_TRACE
	printf("dxactivate\n");
#endif
	elementFunc(x, y, &dxActiveFuncReLU);
}

//step 5
void feedforward(cv::Mat &input, cv::Mat &weight, cv::Mat &bias, cv::Mat &output, cv::Mat &net) {
#ifdef DBG_STACK_TRACE
	printf("feedforward\n");
#endif
	//net = mul(weights, horcat(inputs, bias))
	cv::Mat inbias;
	cv::hconcat(input, bias, inbias);
	net = inbias * weight;
	//output = activate(net)
	activate(net, output);
}

//init random weights for network
void initWeight(float maxWeight, int width, int height, cv::Mat &m) {
#ifdef DBG_STACK_TRACE
	printf("initWeight\n");
#endif
	m = cv::Mat(height, width, CV_32FC1, cv::Scalar(maxWeight));
	elementFunc(m, m, &random);
}

//evaluate the error of the network
void evalError(cv::Mat &input, cv::Mat &weight, cv::Mat &output, cv::Mat &clas, cv::Mat &bias, float &error, float &claserror) {
#ifdef DBG_STACK_TRACE
	printf("evalError\n");
#endif
	cv::Mat net, outs, outclas;

	//[output net] = feedforward(inputs, weights, bias)
	feedforward(input, weight, bias, outs, net);

	//classes = classes_from_output_vectors(outputs)
	outm2classv(outs, outclas);

	//Now subtract the target output matrix from the output matrix, square the components, add together, and normalise
	//error = sum_all_components((target_outputs – outputs)^2) / (sample_count * output_count)
	cv::Mat e;
	cv::subtract(output, outs, e);
	cv::multiply(e, e, e);
	error = cv::sum(e)[0] / float(e.rows * e.cols);
		
	//Count the number of classes that corresponds with the target classes, and divide by the number of samples to normalise
	//c = sum_all_components(classes != target_classes)/sample_count
	cv::Mat c;
	cv::absdiff(outclas, clas, c);
	claserror = cv::sum(c)[0] / float(c.rows * c.cols);
}

//dummy backpropagation function
void backprop(cv::Mat &input, cv::Mat &output, cv::Mat &weight, float learn, cv::Mat &bias, cv::Mat &newWeight) {
#ifdef DBG_STACK_TRACE
	printf("backprop\n");
#endif
	//newWeight = weight;
	
	//First, select a random sample.
#ifdef DBG_LEARN
	int rnd = 0;
#else
	int rnd = int(rndRange(0.f, float(input.rows)));
#endif
	cv::Mat rand_inp = input(cv::Rect(0, rnd, input.cols, 1)),
			rand_out = output(cv::Rect(0, rnd, output.cols, 1)),
			rand_bia = bias(cv::Rect(0, rnd, bias.cols, 1));

	//[output, net] = feedforward(random_sample, weights, random_bias)
	cv::Mat outp, net, err, dlta, dxnet, dxweight, inbias, kro;
	feedforward(rand_inp, weight, rand_bia, outp, net);

	//error_vector = target_outputs - outputs
	cv::subtract(rand_out, outp, err);

	//delta = hammard(error_vector, activation_diff(net))
	dxactivate(net, dxnet);
	cv::multiply(err, dxnet, dlta);

	//weights_delta = learn*kron([inputs(sample_index,:), bias(sample_index)]', delta);
	cv::hconcat(rand_inp, rand_bia, inbias);
	inbias = inbias.t();
	kronecker(inbias, dlta, kro);
	dxweight = kro * learn;

	//weights = add(weights, weights_delta)
	cv::add(weight, dxweight, newWeight);
}


/* Each row has seven entries, separated by tabs. The first four entries are features of
irises(sepal length, sepal width, petal length, and petal width); the last three is
the outputs denoting the species of iris(setosa, versicolor, and virginica). I have
preprocessed the values a bit to get them in the appropriate ranges.
You must read in the data so that you can treat the inputs of each set as a single
matrix; similarly for the outputs. */
void DataSet::load(std::vector<std::string> fileNames, char delim) {
#ifdef DBG_STACK_TRACE
	printf("load\n");
#endif
	std::vector<SubSet *> sets = { &training_set, &validation_set, &test_set };
	for (int fileInd = 0; fileInd < fileNames.size(); ++fileInd) {
		//open the file
		std::vector<std::string> lines;
		bool opened = openTxt(fileNames[fileInd], lines);
		if (!opened) {
			printf("could not open data\n");
			return;
		} else {
			printf("opened %s\n", fileNames[fileInd].c_str());
		}
		//get the subset matching the file
		SubSet &set = *sets[fileInd];
		std::string oneline;
		float *p, *q, *o;
		int ind = 0,
			lastInd = 0,
			numEntries = 7, //should be 7
			j;
		set.training_count = lines.size(),
		input_count = 4;
		output_count = numEntries - input_count;
		
		printf("entries:%i lines:%i width:%i\n", numEntries, set.training_count, input_count);

		set.inputs = cv::Mat::zeros(set.training_count, input_count, CV_32FC1);
		set.outputs = cv::Mat::zeros(set.training_count, output_count, CV_32FC1);

		//read lines once into both matricies?
		for (int i = 0; i < set.training_count; ++i) {
			//split string into floats
			std::vector<float> f(numEntries);
			//read a single line
			oneline = trim(lines[i]);
			lastInd = 0;
			for (j = 0; j < numEntries - 1; ++j) {
				ind = oneline.find_first_of(delim, lastInd);
				f[j] = std::stof(oneline.substr(lastInd, ind));
				lastInd = ind + 1;
			}
			f[numEntries - 1] = std::stof(oneline.substr(lastInd, oneline.length() - lastInd));
			//write to input matrix
			p = set.inputs.ptr<float>(i);
			for (j = 0; j < input_count; ++j) {
				p[j] = f[j];
			}
			//write to output matrix
			q = set.outputs.ptr<float>(i);
			for (j = 0; j < output_count; ++j) {
				q[j] = f[j + input_count];
			}
		}

		set.classes = cv::Mat::zeros(set.training_count, 1, CV_32FC1);
		outm2classv(set.outputs, set.classes);
	}
}


/* The training function should take in three sets, the training_set, validation_set,
and test_set. Implement a way to limit the maximum number of samples that will
actually be used for training. Returns weights and errors */
#ifdef PLOT_GRAPHS
void DataSet::train(cv::Mat &weight, std::vector<float> &te, std::vector<float> &tc, std::vector<float> &ve, std::vector<float> &vc, std::vector<float> &se, std::vector<float> &sc) {
#else
void DataSet::train(cv::Mat &weight) {
#endif
#ifdef DBG_STACK_TRACE
	printf("train\n");
#endif
	//initialise a weight matrix using initialise weights. For now, use a max_weight of 1/2.
	initWeight(.5f, output_count, input_count + 1, weight);

	//The function should also construct three bias vectors bias_training, bias_validate, and bias_test. Each must contain only 1s
	cv::Mat bias_training = cv::Mat::ones(training_set.inputs.rows, 1, CV_32FC1),
		bias_validate = cv::Mat::ones(validation_set.inputs.rows, 1, CV_32FC1),
		bias_test = cv::Mat::ones(test_set.inputs.rows, 1, CV_32FC1);

	int i = 0;
	while (i < 500) {	//limit max samples

		//Use the training set inputs, the weights, (for now) a fixed learning rate of 0.1, and bias vector bias_train. Assign the result to weights.
		backprop(training_set.inputs, training_set.outputs, weight, learnRate, bias_training, weight);

		//call the network error function three times: one time for each of the training, validation, and test sets. Use the weight matrix, and the appropriate bias vector. Wrap these calls in an if-statement that tests for a value plot_graphs
#ifdef PLOT_GRAPHS
		te.push_back(0.f); tc.push_back(0.f); 
		ve.push_back(0.f); vc.push_back(0.f);
		se.push_back(0.f); sc.push_back(0.f);
		evalError(training_set.inputs, weight, training_set.outputs, training_set.classes, bias_training, te.back(), tc.back());
		evalError(validation_set.inputs, weight, validation_set.outputs, validation_set.classes, bias_validate, ve.back(), vc.back());
		evalError(test_set.inputs, weight, test_set.outputs, test_set.classes, bias_test, se.back(), sc.back());
#endif
		++i;
	}

#ifdef PLOT_GRAPHS
	cv::Scalar c1(255, 0, 0), c2(0, 255, 0), c3(0, 0, 255), c4(0, 255, 255), c5(255, 0, 255), c6(255, 255, 0);
	cv::Mat graph = cv::Mat::zeros(500, 500, CV_8UC3);
	cv::Point p;
	std::vector<cv::Point> lp(6);
	float eScale = 26.0f;
	for (i = 0; i < te.size(); ++i) {
		p.x = i;
		//graph training error
		p.y = int(te[i] * eScale * eScale);
		cv::line(graph, lp[0], p, c1);
		lp[0] = p;
		p.y = int(tc[i] * eScale);
		cv::line(graph, lp[3], p, c4);
		lp[3] = p;
		//graph validation error
		p.y = int(ve[i] * eScale * eScale);
		cv::line(graph, lp[1], p, c2);
		lp[1] = p;
		p.y = int(vc[i] * eScale);
		cv::line(graph, lp[4], p, c5);
		lp[4] = p;
		//graph test error
		p.y = int(se[i] * eScale * eScale);
		cv::line(graph, lp[2], p, c3);
		lp[2] = p;
		p.y = int(sc[i] * eScale);
		cv::line(graph, lp[5], p, c6);
		lp[5] = p;
	}
	//show graph
	cv::flip(graph, graph, 0);
	cv::imshow("Error vs Learning Time", graph);
	cv::waitKey(1);
#endif

}


/* The program should load in the sets (using the load_sets function), and pass
these to the training algorithm. */
void DataSet::start(std::vector<std::string> fileNames, char delim) {
	load(fileNames, delim);
	std::vector<float> te, tc, ve, vc, se, sc;
	train(_weights, te, tc, ve, vc, se, sc);
}
