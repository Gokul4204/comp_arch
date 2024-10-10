#include <bits/stdc++.h>
using namespace std;

int M;
int N;
string trace_file;

class BranchPredictor {
    int M;
    int N;
    int globalHistoryRegister;
    int globalHistoryMask;
    int indexMask;
    map<int, int> branchPredictionBuffer;
    int missPredictions;
    int totalPredictions;
    string predictorType;

    private:

    void makePrediction(int index, int outcome) {
        int prediction = 1;
        if (branchPredictionBuffer.find(index) == branchPredictionBuffer.end()) {
            branchPredictionBuffer[index] = 2;
        }
        if (branchPredictionBuffer[index] < 2) {
            prediction = -1;
        }
        if (prediction != outcome) {
            missPredictions++;
        }
        totalPredictions++;
    }

    void updatePredictionBuffer(int index, int outcome) {
        branchPredictionBuffer[index] = max(min(branchPredictionBuffer[index] + outcome, 3), 0);
    }

    void updateGlobalHistoryRegister(int outcome) {
        outcome = max(outcome, 0);
        globalHistoryRegister = (globalHistoryRegister >> 1) | (outcome << (N - 1));
        globalHistoryRegister &= globalHistoryMask;
    }

    public:

    BranchPredictor(int M, int N, string predictorType) {
        this->M = M;
        this->N = N;
        this->globalHistoryRegister = 0;
        this->globalHistoryMask = (1 << N) - 1;
        this->indexMask = (1 << M) - 1;
        this->missPredictions = 0;
        this->totalPredictions = 0;
        this->predictorType = predictorType;
    }

    void runPredictor(string PC, string outcome_) {
        int significantPCBits = (stoi(PC, 0, 16) >> 2) & indexMask;
        int ghr = globalHistoryRegister;
        int index = (ghr << (M - N)) ^ significantPCBits;
        int outcome = outcome_ == "t" ? 1 : -1;
        makePrediction(index, outcome);
        updatePredictionBuffer(index, outcome);
        updateGlobalHistoryRegister(outcome);
    }

    void printResults() {
        cout << "OUTPUT" << endl;
        cout << "number of predictions: " << totalPredictions << endl;
        cout << "number of mispredictions: " << missPredictions << endl;
        cout << "misprediction rate: " << fixed << setprecision(2) << (double)missPredictions / totalPredictions * 100 << "%" << endl;
    }

    void printPredictionBuffer() {
        cout << "FINAL " << predictorType << " CONTENTS" << endl;
        for(int i=0; i<(1 << M); i++) {
            int prediction = 2;
            if (branchPredictionBuffer.find(i) != branchPredictionBuffer.end()) {
                prediction = branchPredictionBuffer[i];
            }
            cout << i << " " << prediction << endl;
        }
    }
};

void print_usage() {
    cerr << "Bimodal predictor Usage: ./bpsim bimodal <M> <trace_file>" << endl;
    cerr << "Gshare predictor Usage: ./bpsim gshare <M> <N> <trace_file>" << endl;
}

int main(int argc, char *argv[]) {

    cout << argc << endl;
    
    if (argc < 4) {
        print_usage();
        return 1;  
    }

    string predictorType = argv[1];

    if (predictorType == "bimodal" && argc == 4) {
        cout << "COMMAND" << endl;
        cout << "./bpsim" << " " << argv[1] << " " << argv[2] << " " << argv[3] << endl;
    } else if (predictorType == "gshare" && argc == 5) {
        cout << "COMMAND" << endl;
        cout << "./bpsim" << " " << argv[1] << " " << argv[2] << " " << argv[3] << " " << argv[4] << endl;
    } else {
        print_usage();
        return 1; 
    }

    M = stoi(argv[2]);
    predictorType == "gshare" ? N = stoi(argv[3]) : N = 0;
    predictorType == "gshare" ? trace_file = argv[4] : trace_file = argv[3];

    ifstream traceFile(trace_file); 
  
    if (!traceFile.is_open()) { 
        cerr << "Error opening the trace file!" << endl; 
        return 1; 
    } 

    BranchPredictor bp(M, N, predictorType);
  
    string line; 
  
    while (getline(traceFile, line)) { 
        istringstream iss(line); 
        string PC, outcome;
        iss >> PC >> outcome;
        bp.runPredictor(PC, outcome);
    }

    bp.printResults();
    bp.printPredictionBuffer();


    return 0;
}