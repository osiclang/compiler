package eval

import (
	"olang/pkg/libs/ml/regression/learn"
	"olang/pkg/libs/ml/regression/learn/slice"
	"olang/pkg/libs/ml/regression/learn/vectorized"
	"olang/pkg/libs/ml/regression/predict"
	"olang/pkg/object"
	"olang/pkg/token"
)

var neuronals = map[string]*object.Neuronal{
	"linRegres": {Fn: linearRegression},
}

// linRegres(alpha int, iterations int, isVectorized bool)
// performs a linearRegression on a set two dimensional set of values
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

	// TODO, we need matricies now!

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

	elems := make([]object.Object, len(float64s), len(float64s))
	i := 0
	for curVal := range float64s {
		elems[i] = &object.Float{Value: curVal}
		i++
	}
	return &object.Array{Elements: elems}
}
