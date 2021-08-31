package eval

import (
	"encoding/json"
	"errors"
	"fmt"
	"olang/pkg/logics"
	"olang/pkg/object"
	"olang/pkg/token"
)

var neuroSet logics.Cells
var goals []logics.Goal

var neuronals = map[string]*object.Neuronal{
	"createNeuronet": {Fn: createNeuro},
	"setReactor":     {Fn: setReactor},
	"setActor":       {Fn: setReactor},
	"setGoal":        {Fn: setGoal},
	"getMindSet":     {Fn: getMindSet},
	"evolve":         {Fn: evolve},
}

// creates a new plain network fron neurons
// the size is calculated by a integer
// example createNeuronet(10) creates a network that is 10*10*10 neurons
func createNeuro(args ...object.Object) object.Object {
	if len(args) == 0 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected 1", len(args))
	} else if len(args) == 1 {
		switch arg := args[0].(type) {
		case *object.Integer:
			createNetwork(arg)
		default:
			return newError(token.Position{}, "wrong arg types")
		}

	} else {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected 1", len(args))
	}
	return ConstNil
}

// this is stupid code to create a pattern of x*x*x cells
// the cells are connected to each naighbor but unconfigured
// The cell have to learn by her own, what the best way is, to propergate a signal
func createNetwork(arg *object.Integer) {
	for i := int64(0); i < int64(arg.Value); i++ {
		for ii := int64(0); ii < int64(arg.Value); ii++ {
			for iii := int64(0); iii < int64(arg.Value); iii++ {
				a := logics.Cell{
					X:         i,
					Y:         ii,
					Z:         iii,
					ID:        createCellId(i, ii, iii),
					Constrain: []logics.Constrain{},
				}
				c, _ := createConnector(i, ii, iii, a, logics.ReceiverType(logics.Signal))
				a.Constrain = append(a.Constrain, c)

				neuroSet.Cells = append(neuroSet.Cells, a)
			}
		}
	}
}

//create a contrain to a other cell, identified by x,y,z
func createConnector(x int64, y int64, z int64, a logics.Cell, receiverType logics.ReceiverType) (logics.Constrain, error) {
	if len(a.Constrain) <= 7 {
		c := logics.Constrain{
			ReceiverType: receiverType,
			DestID:       createCellId(x, y, z),
			Score:        0,
			Deployrule:   []logics.Deployrule{},
		}

		dc, err := getCellById(createCellId(x, y, z))
		if err != nil || len(dc.Constrain) > 7 {
			return logics.Constrain{}, errors.New(fmt.Sprintf("cannot create connection for %s", createCellId(x, y, z)))
		}

		dc.Constrain = append(dc.Constrain, logics.Constrain{
			ReceiverType: receiverType,
			DestID:       createCellId(a.X, a.Y, a.Z),
			Score:        0,
		})

		return c, nil
	}
	return logics.Constrain{}, errors.New(fmt.Sprintf("cannot create connection for %s", createCellId(x, y, z)))
}

func getCellById(id string) (logics.Cell, error) {
	for _, e := range neuroSet.Cells {
		if e.ID == id {
			return e, nil
		}
	}
	return logics.Cell{}, errors.New(fmt.Sprintf("cannot find cell %s", id))
}

// here we can set an actor, that can create an output (actor, because the system acts)
func setActor(args ...object.Object) object.Object {
	if len(args) != 4 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected 4", len(args))
	}
	dc, err := getCellById(createCellId(getIntegerType(args[0]), getIntegerType(args[1]), getIntegerType(args[2])))
	if err != nil || len(dc.Constrain) > 7 {
		return newError(token.Position{}, "The destinated cell is not found or cannot add one more contrain")
	}

	dc.Constrain = append(dc.Constrain, logics.Constrain{
		ReceiverType: logics.ReceiverType(logics.Signal),
		DestID:       string(getIntegerType(args[3])),
		Score:        999,
	})

	neuroSet.Actor = append(neuroSet.Actor, logics.Actor{
		Id:     getIntegerType(args[3]),
		DestId: createCellId(getIntegerType(args[0]), getIntegerType(args[1]), getIntegerType(args[2])),
	})

	return ConstNil
}

// here we can set an reactor, that can create a signal from outside
func setReactor(args ...object.Object) object.Object {
	if len(args) != 4 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected 4", len(args))
	}
	dc, err := getCellById(createCellId(getIntegerType(args[0]), getIntegerType(args[1]), getIntegerType(args[2])))
	if err != nil || len(dc.Constrain) > 7 {
		return newError(token.Position{}, "The destinated cell is not found or cannot add one more contrain")
	}

	dc.Constrain = append(dc.Constrain, logics.Constrain{
		ReceiverType: logics.ReceiverType(logics.Signal),
		DestID:       string(getIntegerType(args[3])),
		Score:        999,
	})

	neuroSet.Reactor = append(neuroSet.Reactor, logics.Reactor{
		Id:     getIntegerType(args[3]),
		DestId: createCellId(getIntegerType(args[0]), getIntegerType(args[1]), getIntegerType(args[2])),
	})

	return ConstNil
}

// here we can add a goal.
// you can add as much goals as you want
func setGoal(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError(token.Position{}, "wrong number of arguments. got '%d', expected 2", len(args))
	}

	var actor int64
	var reactor int64

	switch arg := args[0].(type) {
	case *object.Integer:
		actor = arg.Value
	default:
		return newError(token.Position{}, "1st argument must be a type of integer ")
	}

	switch arg := args[1].(type) {
	case *object.Integer:
		reactor = arg.Value
	default:
		return newError(token.Position{}, "2nd argument must be a type of integer ")
	}

	goals = append(goals, logics.Goal{
		Reactor: reactor,
		Actor:   actor,
	})

	return ConstNil
}

//here the magic happends
//the system will now try to arrage the cells and his contraints until all goals are satisfied
func evolve(args ...object.Object) object.Object {
	if len(args) != 0 {
		return newError(token.Position{}, "wrong number of arguments. got '%d' but expected no arguments", len(args))
	}
	logics.EvolveLogic(neuroSet, goals)
	return ConstNil
}

// returns a JSON presentation of the existing cells with all his rules and contrains
func getMindSet(args ...object.Object) object.Object {
	s, _ := json.MarshalIndent(neuroSet, "", " ")
	return &object.String{Value: string(s)}
}

func createCellId(x int64, y int64, z int64) string {
	return fmt.Sprintf("%v, %v, %v", x, y, z)
}

func getIntegerType(arg ...object.Object) int64 {
	switch arg := arg[0].(type) {
	case *object.Integer:
		return arg.Value
	default:
		return 0
	}
}
