package slice

import (
	"encoding/csv"
	"olang/pkg/libs/ml/regression/gradient"
	"olang/pkg/libs/ml/regression/learn"
	"olang/pkg/libs/ml/regression/normalize"
	"olang/pkg/libs/ml/regression/predict"
	"olang/pkg/libs/ml/regression/predict/slice"
	"os"
	"strconv"
)

type learnSlice struct {
}

func NewlearnSlice() learn.Learn {
	return &learnSlice{}
}

// Init Slices with csv file input
func (l *learnSlice) Learn(config learn.LearnConfiguration) (predict.Predict, error) {

	inputs, y, err := loadFile(config.TrainingFileName)
	if err != nil {
		return nil, err
	}

	theta := make([]float64, len(inputs[0])+1)

	// Normalize all the elements to keep an identical scale between different data
	XNorm, M, S, err := normalize.Normalize(inputs)

	// Perform gradient descent to calculate Theta
	theta, err = gradient.LinearGradient(XNorm, y, theta, config.Alpha, config.NumberIteration)
	if err != nil {
		return nil, err
	}

	return slice.NewSlicePredict(config.PredictionFileName, theta, M, S)
}

func loadFile(fileName string) ([][]float64, []float64, error) {
	f, err := os.Open(fileName)
	if err != nil {
		return nil, nil, err
	}
	defer f.Close()

	lines, err := csv.NewReader(f).ReadAll()
	if err != nil {
		return nil, nil, err
	}

	inputs := make([][]float64, len(lines))
	y := make([]float64, len(lines))

	// Loop through lines & turn into object
	for i, line := range lines {
		inputs[i] = make([]float64, len(line)-1)
		for j, data := range line {
			f, err := strconv.ParseFloat(data, 64)
			if err != nil {
				return nil, nil, err
			}

			if err != nil {
				return nil, nil, err
			}
			if j < len(line)-1 {
				inputs[i][j] = f
			} else {
				y[i] = f
			}

		}

	}
	return inputs, y, nil
}
