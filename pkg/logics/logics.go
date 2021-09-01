package logics

import (
	"errors"
	"fmt"
	"io"
	"math/rand"
	"os"
	"strconv"
	"sync"
	"time"
)

var OutStream io.Writer = os.Stdout

var propagationList []propagationQueue

type ReceiverType int64

// here we can add Signal
// Signal just means a plain bit/ping that is send to transfer
// Good, Bad and other to come are types that create special reactions to a Cell
const (
	Neutral ReceiverType = iota
	Signal
	Good
	Bad
)

type Cells struct {
	Cells   []Cell
	Actor   []Actor
	Reactor []Reactor
}

type Cell struct {
	X         int64
	Y         int64
	Z         int64
	ID        string
	Constrain []Constrain
}

type Actor struct {
	Id     int64
	DestId string
}

type Reactor struct {
	Id     int64
	DestId string
}

type Constrain struct {
	ReceiverType ReceiverType
	DestID       string
	Score        int64
	Deployrule   []Deployrule
}

type Deployrule struct {
	DestID   string
	OutputID string
}

type Goal struct {
	Actor    int64
	Reactor  int64
	SignalId int64
}

type propagationQueue struct {
	Sender        string
	Receipient    string
	SignalId      int
	ReceiverTypus ReceiverType
}

func EvolveLogic(neuroSet Cells, goals []Goal) {
	var wg sync.WaitGroup
	wg.Add(len(neuroSet.Cells))
	for _, e := range neuroSet.Cells {
		go cellClock(e)
	}
	wg.Wait()

}

func cellClock(cell Cell) {
	for {
		fmt.Fprintln(OutStream, cell.ID)
		amt := time.Duration(rand.Intn(250))
		time.Sleep(time.Millisecond * amt)
	}
}

func propagateSignalFromGoals(neuroSet Cells, goals []Goal) {
	for _, g := range goals {
		for _, a := range neuroSet.Actor {
			if g.Actor == a.Id {
				propagationList = append(propagationList, propagationQueue{
					Sender:        strconv.FormatInt(a.Id, 10),
					Receipient:    a.DestId,
					SignalId:      rand.Int(),
					ReceiverTypus: ReceiverType(Signal),
				})
			}
		}
	}
}

func lookupSignal(cellid string) (propagationQueue, error) {
	for _, p := range propagationList {
		if p.Receipient == cellid {
			return p, nil
		}
	}
	return propagationQueue{}, errors.New(fmt.Sprintf("cannot find cell %s", cellid))
}

func removeFromProgationQueue(p propagationQueue) {
	for i, list := range propagationList {
		if p.Receipient == list.Receipient && p.Sender == list.Sender {
			removeIndex(i)
		}
	}
}

func removeIndex(index int) {
	propagationList = append(propagationList[:index], propagationList[index+1:]...)
}
