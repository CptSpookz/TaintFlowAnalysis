#ifndef DEPGRAPH_H_
#define DEPGRAPH_H_

#ifndef DEBUG_TYPE
#define DEBUG_TYPE "depgraph"
#endif

#define USE_ALIAS_SETS true
#define useCallStrings true
#define SensitivityDepth  0

#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/ADT/GraphTraits.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Analysis/DominanceFrontier.h"
#include "llvm/Analysis/MemoryDependenceAnalysis.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/CallSite.h"
#include "llvm/ADT/SCCIterator.h"
#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/CallGraphSCCPass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/GraphWriter.h"
#include "llvm/Support/raw_ostream.h"
//#include "../AliasSets/AliasSets.h"
#include <deque>
#include <algorithm>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include "llvm/IR/DebugInfo.h"
#include "dsa/DataStructure.h"
#include "dsa/DSGraph.h"
#include "FlowGraph.h"
#include "InputDep.h"
#include "CallGraphWrapper.h"

using namespace std;

namespace llvm {

/*
 * Class functionDepGraph
 *
 * Function pass that provides an intraprocedural dependency graph
 *
 */
class functionDepGraph: public FunctionPass {
public:
        static char ID; // Pass identification, replacement for typeid.
        functionDepGraph() :
                FunctionPass(ID), depGraph(NULL) {
        }
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnFunction(Function&);

        Graph* depGraph;
};



class RelevantFields {
public:
    std::string functionName;
    std::string variable;
    std::string fieldName;
    int index;
    bool isStruct;
    RelevantFields();
  //  void Print();
};


/*
 * Class moduleDepGraph
 *
 * Module pass that provides a context-insensitive interprocedural dependency graph
 *
 */
class moduleDepGraph: public ModulePass {
public:
        static char ID; // Pass identification, replacement for typeid.
        moduleDepGraph() :
                ModulePass(ID), depGraph(NULL) {
        }
        void getAnalysisUsage(AnalysisUsage &AU) const;
        bool runOnModule(Module&);



        // Individual DefValue..
        pair<BasicBlock*, Value*> DefValue;
        std::set<Value*> inputDepValues;
        std::map<CallSite, std::vector<Function*> > IndMap;
        MemoryDependenceAnalysis * MDA;
        set<BasicBlock*> ProcessedBlocks;
        set<BasicBlock*> BT_ProcessedBlocks;
        //map from the pointer value to the set of places where the store happens and from what Val..
        std::map<Value*,set<pair<BasicBlock*, Value*> > > RDS_map;
        map<Function*, int> FunctionGraphGen;

        set<CallSiteMap*> CallSitesProcessed;
        set<CallSiteMap*> CallParamsMap;
        map<CallSite*,map<Value*,set<MemoryOperation*> > >  callSiteLiveParams;
        //map to store the points to graph for each function.
        map<Function*,DSGraph*> funcPointsToMap;
        DSGraph* globalPointsToGraph;
        vector<Function*> callStringFunctions;
        map<Function*, set< vector<Function*> > > ProcessedCallStringFunctions;
        string currentCallString;
        map<Function*, set<string> >  ProcessedCallStrings;

        std::set<RelevantFields*> relevantFields;
        std::set<Value*> FieldVals;

        PathSet callPaths;

        ConfigInfo configData;


        vector<string> callPathFunctions;



        void Process_Functions(Function* F, Graph* F_Graph, int SensDepth);
        set<BasicBlock*>  Process_Block(BasicBlock* BB, Graph* F_Graph);
        void CollectLiveStores(BasicBlock* parentBlock, Instruction * Linst,Graph* F_Graph);
        void CollectLiveStores_Params(BasicBlock* predBlock, Instruction * Cinst,Value* pointer, Graph* F_Graph);
        void matchParametersAndReturnValues(Function &F,Graph * Full_Graph);
        void matchParametersAndReturnValues_new(Function &F);
        bool checkPointer(Value * Ptr, Value* targetPtr, Function * parentFunc);
       // void deleteCallNodes(Function* F);
        void AddUniqueQueueItem(set<BasicBlock*> &setBlock, queue<BasicBlock*> &queueBlock, BasicBlock * bb);

        void TopDownProcessing(Function * F,AliasSets * AS);
        void Process_CallSite(Instruction* inst, Function *F);
        void HandleLibraryFunctions(CallInst * CI, Function* F);

        int isCallSiteProcessed(CallSiteMap* csm);
        void getcallPathFunctions();
        bool isFunctiononCallPath(Function * F);
        bool isFunctiononCallString(Function * F);
        void InitializePointtoGraphs(Module&,BUDataStructures&);
        bool isPathProcessed(Function* F, vector<Function*> callPath, CallSite callSite);

        void ForwardProcess(Function* F, Graph * F_Graph);

        void MatchContextParams(Function* Caller, Function *Callee, CallInst *CI,Graph* F_Graph);

        void ProcessGlobals(Module& M, Graph* TopDownGraph);

        void LoadStoreMap(map<Value*, set< Value*> > LoadQueue,BasicBlock* TermBlock,Graph * F_Graph);
        void BFSLoadStoreMap(map<Value*, set< Value*> > LoadQueue,Function * F, Graph * F_Graph);

        void ReadRelevantFields();

        void CheckifRelevantField(GetElementPtrInst * GI,Graph* F_Graph);


        std::vector<instructionCallSite*> ProcessedCallsites;
        std::map<Function*,Graph*> FuncDepGraphs;
        std::pair<BasicBlock*,int> defLoc;
        std::map<Value*,vector<pair<BasicBlock*,int> > > RDS;

        Graph* depGraph;
        Graph * TopDownGraph;
        Graph *FullGraph;


        //Functions for replication Function pointer target info:
        std::set<Function*> addressTaken;
        int nodeCount;
        std::vector<Function*> calledFuncs;
        std::set<CallSite> unResolved;
        std::set<CallSite> CompleteSites;
        std::list<CallSite> AllSites;

        void ComputeIndMap(Module  &M);
        std::vector<Function*> getPossibleCallees(CallSite cs);
};



class ViewModuleDepGraph: public ModulePass {
public:
        static char ID; // Pass identification, replacement for typeid.
        ViewModuleDepGraph() :
                ModulePass(ID) {
        }

        void getAnalysisUsage(AnalysisUsage &AU) const {
                AU.addRequired<moduleDepGraph> ();
                AU.setPreservesAll();
        }

        bool runOnModule(Module& M) {

                moduleDepGraph& DepGraph = getAnalysis<moduleDepGraph> ();
                Graph *g = DepGraph.depGraph;

                std::string tmp = M.getModuleIdentifier();
                replace(tmp.begin(), tmp.end(), '\\', '_');

                std::string Filename = "/tmp/" + tmp + ".dot";

                //Print dependency graph (in dot format)
                g->toDot(M.getModuleIdentifier(), Filename);

                //DisplayGraph(Filename, true, GraphProgram::DOT);

                return false;
        }
};
}

#endif //DEPGRAPH_H_
