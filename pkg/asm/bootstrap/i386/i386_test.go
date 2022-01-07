package i386

import (
	"olang/pkg/asm/objects"
	"olang/pkg/asm/objectsmap"
	"testing"
)

func TestAddPrintBs(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addPrintBs(&testSq)
	want := objects.PRINT_CLI
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	seqmentName := testSq.OpsSgmnt[0].Segment
	if want != seqmentName {
		t.Fatalf(`addPrintBs = %q, want match for %#q, nil`, seqmentName, want)
	}
}

func TestAddCliInputBs(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addCliInputBs(&testSq)
	want := objects.INPUT_CLI
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addCliInputBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	seqmentName := testSq.OpsSgmnt[0].Segment
	if want != seqmentName {
		t.Fatalf(`addCliInputBs = %q, want match for %#q, nil`, seqmentName, want)
	}
}

func TestAddMulBs(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addMulBs(&testSq)
	want := objects.MUL_INT
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addMulBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	seqmentName := testSq.OpsSgmnt[0].Segment
	if want != seqmentName {
		t.Fatalf(`addMulBs = %q, want match for %#q, nil`, seqmentName, want)
	}
}

func TestAddDivBs(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addDivBs(&testSq)
	want := objects.DIV_INT
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addDivBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	seqmentName := testSq.OpsSgmnt[0].Segment
	if want != seqmentName {
		t.Fatalf(`addDivBs = %q, want match for %#q, nil`, seqmentName, want)
	}
}

func TestCheckPersistence(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addPrintBs(&testSq)
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	addPrintBs(&testSq)
	if len(testSq.OpsSgmnt) != 1 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 1 got %d`, len(testSq.OpsSgmnt))
	}
}

func TestAppendOpTSeg(t *testing.T) {
	testSq := objectsmap.OpsSegments{}
	addPrintBs(&testSq)
	if len(testSq.OpsSgmnt) == 0 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 1`)
	}
	addPrintBs(&testSq)
	if len(testSq.OpsSgmnt) != 1 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 1 got %d`, len(testSq.OpsSgmnt))
	}
	addDivBs(&testSq)
	if len(testSq.OpsSgmnt) != 2 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt), want match for 2 got %d`, len(testSq.OpsSgmnt))
	}
	appendOpTSeg(&testSq, objects.PRINT_CLI, objects.BL, []string{"0"})
	if len(testSq.OpsSgmnt[0].Ops) != 7 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt[0].Ops), want match for 7 got %d`, len(testSq.OpsSgmnt[0].Ops))
	}
	if len(testSq.OpsSgmnt[1].Ops) != 5 {
		t.Fatalf(`addPrintBs = len(testSq.OpsSgmnt[0].Ops), want match for 5 got %d`, len(testSq.OpsSgmnt[1].Ops))
	}
}
