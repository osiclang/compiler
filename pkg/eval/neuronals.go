package eval

import (
	"flag"
	"olang/pkg/libs/ml/regression/learn"
	"olang/pkg/libs/ml/regression/learn/slice"
	"olang/pkg/libs/ml/regression/learn/vectorized"
	"olang/pkg/libs/ml/regression/predict"
	"olang/pkg/object"
	"olang/pkg/token"
)

var neuronals = map[string]*object.Neuronal{
	"linRegres":       {Fn: linearRegression},
	"linRegresOnFile": {Fn: linearRegressionOnFile},
}

func linearRegression(args ...object.Object) object.Object {

	var isVectorized bool
	var alpha float64
	var iteration int

	if len(args) != 3 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '3'", len(args))
	}

	switch arg := args[0].(type) {
	case *object.Float:
		alpha = arg.Value
	default:
		return newError(token.Position{}, "alpha(steps) must be float, got '%s'", args[0].Type())
	}

	switch arg := args[1].(type) {
	case *object.Integer:
		iteration = int(arg.Value)
	default:
		return newError(token.Position{}, "iteration must be Integer, got '%s'", args[0].Type())
	}

	switch arg := args[1].(type) {
	case *object.Boolean:
		isVectorized = arg.Value
	default:
		return newError(token.Position{}, "isVectorized must be Boolean, got '%s'", args[0].Type())
	}

	config := learn.LearnConfiguration{
		NumberIteration: iteration,
		Alpha:           alpha,
	}
	var predictO predict.Predict
	var learnO learn.Learn
	if isVectorized {
		learnO = vectorized.NewlearnVectorized()
	} else {
		learnO = slice.NewlearnSlice()
	}
	predictO, err := learnO.Learn(config)
	if err != nil {
		return newError(token.Position{}, "Error on linearRegression")
	}
	length := predictO.PredictLength()
	float64s := make(chan float64, length)
	go predictO.Predict(float64s)

	//writeData(*resultFile, float64s)
}

func linearRegressionOnFile(args ...object.Object) object.Object {

	if len(args) != 3 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected '3'", len(args))
	}

	//trainingFile := flag.String("training_file_path", "data_simple.txt", "a training csv file ")
	alpha := flag.Float64("alpha_value", 0.2, "gradient step")
	iteration := flag.Int("iteration_number", 1000, "training iteration")
	isVectorized := flag.Bool("vectorized_version", false, "")

	config := learn.LearnConfiguration{
		NumberIteration: *iteration,
		Alpha:           *alpha,
	}
	var predictO predict.Predict
	var learnO learn.Learn
	if *isVectorized {
		learnO = vectorized.NewlearnVectorized()
	} else {
		learnO = slice.NewlearnSlice()
	}
	predictO, err := learnO.Learn(config)
	if err != nil {
		return newError(token.Position{}, "Error on linearRegression")
	}
	length := predictO.PredictLength()
	float64s := make(chan float64, length)
	go predictO.Predict(float64s)

	//writeData(*resultFile, float64s)
}
