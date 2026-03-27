#include "Narrative/PangeaNarrativeGraphRebuilder.h"

#if WITH_EDITOR

#include "DialogueBlueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphSchema.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "QuestBlueprint.h"
#include "Tales/Dialogue.h"
#include "Tales/DialogueSM.h"
#include "Tales/Quest.h"
#include "Tales/QuestSM.h"

namespace
{
	const TCHAR* QuestGraphClassPath = TEXT("/Script/NarrativeQuestEditor.QuestGraph");
	const TCHAR* QuestGraphSchemaClassPath = TEXT("/Script/NarrativeQuestEditor.QuestGraphSchema");
	const TCHAR* QuestRootNodeClassPath = TEXT("/Script/NarrativeQuestEditor.QuestGraphNode_Root");
	const TCHAR* QuestStateNodeClassPath = TEXT("/Script/NarrativeQuestEditor.QuestGraphNode_State");
	const TCHAR* QuestActionNodeClassPath = TEXT("/Script/NarrativeQuestEditor.QuestGraphNode_Action");

	const TCHAR* DialogueGraphClassPath = TEXT("/Script/NarrativeDialogueEditor.DialogueGraph");
	const TCHAR* DialogueGraphSchemaClassPath = TEXT("/Script/NarrativeDialogueEditor.DialogueGraphSchema");
	const TCHAR* DialogueRootNodeClassPath = TEXT("/Script/NarrativeDialogueEditor.DialogueGraphNode_Root");
	const TCHAR* DialogueNPCNodeClassPath = TEXT("/Script/NarrativeDialogueEditor.DialogueGraphNode_NPC");
	const TCHAR* DialoguePlayerNodeClassPath = TEXT("/Script/NarrativeDialogueEditor.DialogueGraphNode_Player");

	constexpr int32 StateColumnX = 0;
	constexpr int32 ActionColumnX = 620;
	constexpr int32 RowSpacingY = 260;
	constexpr int32 BranchOffsetY = 120;

	template <typename T>
	T* LoadRequiredClass(const TCHAR* ClassPath)
	{
		return LoadObject<T>(nullptr, ClassPath);
	}

	bool SetObjectProperty(UObject* Target, const FName PropertyName, UObject* Value)
	{
		if (!Target)
		{
			return false;
		}

		if (FObjectPropertyBase* Property = FindFProperty<FObjectPropertyBase>(Target->GetClass(), PropertyName))
		{
			Property->SetObjectPropertyValue_InContainer(Target, Value);
			return true;
		}

		return false;
	}

	UEdGraphPin* FindPin(const UEdGraphNode* Node, const EEdGraphPinDirection Direction, const int32 Index = 0)
	{
		if (!Node)
		{
			return nullptr;
		}

		int32 FoundPins = 0;
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin && Pin->Direction == Direction)
			{
				if (FoundPins == Index)
				{
					return Pin;
				}

				++FoundPins;
			}
		}

		return nullptr;
	}

	void BreakAllLinks(UEdGraphNode* Node)
	{
		if (!Node)
		{
			return;
		}

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin)
			{
				Pin->BreakAllPinLinks();
			}
		}
	}

	void ConfigureExistingNode(UEdGraphNode* Node, UObject* PrimaryObject, const int32 X, const int32 Y)
	{
		if (!Node || !PrimaryObject)
		{
			return;
		}

		Node->Modify();
		BreakAllLinks(Node);
		SetObjectProperty(Node, TEXT("QuestNode"), PrimaryObject);
		SetObjectProperty(Node, TEXT("State"), PrimaryObject);
		SetObjectProperty(Node, TEXT("Branch"), PrimaryObject);
		SetObjectProperty(Node, TEXT("DialogueNode"), PrimaryObject);
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Node->SnapToGrid(16);
	}

	UEdGraphNode* CreateGraphNode(UEdGraph* Graph, UClass* NodeClass, UObject* PrimaryObject, const int32 X, const int32 Y)
	{
		if (!Graph || !NodeClass || !PrimaryObject)
		{
			return nullptr;
		}

		Graph->Modify();

		UEdGraphNode* Node = NewObject<UEdGraphNode>(Graph, NodeClass, NAME_None, RF_Transactional);
		if (!Node)
		{
			return nullptr;
		}

		SetObjectProperty(Node, TEXT("QuestNode"), PrimaryObject);
		SetObjectProperty(Node, TEXT("State"), PrimaryObject);
		SetObjectProperty(Node, TEXT("Branch"), PrimaryObject);
		SetObjectProperty(Node, TEXT("DialogueNode"), PrimaryObject);

		Node->Rename(nullptr, Graph, REN_NonTransactional);
		Graph->AddNode(Node, true);
		Node->CreateNewGuid();
		Node->PostPlacedNewNode();
		Node->NodePosX = X;
		Node->NodePosY = Y;
		Node->SnapToGrid(16);
		Node->AllocateDefaultPins();
		return Node;
	}

	bool ConnectNodes(UEdGraph* Graph, UEdGraphNode* FromNode, UEdGraphNode* ToNode)
	{
		if (!Graph || !FromNode || !ToNode)
		{
			return false;
		}

		UEdGraphPin* FromPin = FindPin(FromNode, EGPD_Output);
		UEdGraphPin* ToPin = FindPin(ToNode, EGPD_Input);
		const UEdGraphSchema* Schema = Graph->GetSchema();

		if (!FromPin || !ToPin || !Schema)
		{
			return false;
		}

		return Schema->TryCreateConnection(FromPin, ToPin);
	}

	UEdGraph* EnsureGraph(UBlueprint* Blueprint, const TCHAR* GraphClassPath, const TCHAR* SchemaClassPath, const TCHAR* GraphName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}

		UEdGraph* Graph = nullptr;
		if (UQuestBlueprint* QuestBlueprint = Cast<UQuestBlueprint>(Blueprint))
		{
			Graph = QuestBlueprint->QuestGraph;
		}
		else if (UDialogueBlueprint* DialogueBlueprint = Cast<UDialogueBlueprint>(Blueprint))
		{
			Graph = DialogueBlueprint->DialogueGraph;
		}

		if (Graph)
		{
			return Graph;
		}

		UClass* GraphClass = LoadRequiredClass<UClass>(GraphClassPath);
		UClass* SchemaClass = LoadRequiredClass<UClass>(SchemaClassPath);
		if (!GraphClass || !SchemaClass)
		{
			return nullptr;
		}

		Graph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, GraphName, GraphClass, SchemaClass);
		FBlueprintEditorUtils::AddUbergraphPage(Blueprint, Graph);

		if (const UEdGraphSchema* Schema = Graph->GetSchema())
		{
			Schema->CreateDefaultNodesForGraph(*Graph);
		}

		if (UQuestBlueprint* QuestBlueprint = Cast<UQuestBlueprint>(Blueprint))
		{
			QuestBlueprint->QuestGraph = Graph;
		}
		else if (UDialogueBlueprint* DialogueBlueprint = Cast<UDialogueBlueprint>(Blueprint))
		{
			DialogueBlueprint->DialogueGraph = Graph;
		}

		return Graph;
	}

	struct FQuestLayoutData
	{
		TArray<UQuestState*> OrderedStates;
		TMap<UQuestState*, int32> StateRows;
		TMap<UQuestBranch*, int32> BranchRows;
		TMap<UQuestState*, TArray<UQuestBranch*>> StateBranches;
		TMap<UQuestBranch*, UQuestState*> BranchDestinations;
	};

	FQuestLayoutData BuildQuestLayout(UQuest* Quest)
	{
		FQuestLayoutData Layout;
		if (!Quest || !Quest->GetQuestStartState())
		{
			return Layout;
		}

		TSet<UQuestState*> VisitedStates;
		TQueue<UQuestState*> Queue;
		Queue.Enqueue(Quest->GetQuestStartState());
		VisitedStates.Add(Quest->GetQuestStartState());

		while (!Queue.IsEmpty())
		{
			UQuestState* State = nullptr;
			Queue.Dequeue(State);
			if (!State)
			{
				continue;
			}

			Layout.StateRows.Add(State, Layout.OrderedStates.Num());
			Layout.OrderedStates.Add(State);

			const TArray<UQuestBranch*>& Branches = State->Branches;
			Layout.StateBranches.Add(State, Branches);

			for (const UQuestBranch* Branch : Branches)
			{
				if (!Branch)
				{
					continue;
				}

				Layout.BranchRows.Add(const_cast<UQuestBranch*>(Branch), Layout.StateRows[State]);
				Layout.BranchDestinations.Add(const_cast<UQuestBranch*>(Branch), Branch->DestinationState);

				if (Branch->DestinationState && !VisitedStates.Contains(Branch->DestinationState))
				{
					VisitedStates.Add(Branch->DestinationState);
					Queue.Enqueue(Branch->DestinationState);
				}
			}
		}

		for (UQuestState* State : Quest->GetStates())
		{
			if (State && !Layout.StateRows.Contains(State))
			{
				Layout.StateRows.Add(State, Layout.OrderedStates.Num());
				Layout.OrderedStates.Add(State);
				Layout.StateBranches.Add(State, State->Branches);

				for (UQuestBranch* Branch : State->Branches)
				{
					if (Branch && !Layout.BranchRows.Contains(Branch))
					{
						Layout.BranchRows.Add(Branch, Layout.StateRows[State]);
						Layout.BranchDestinations.Add(Branch, Branch->DestinationState);
					}
				}
			}
		}

		return Layout;
	}

	bool RebuildQuestGraphInternal(UQuestBlueprint* QuestBlueprint)
	{
		if (!QuestBlueprint || !QuestBlueprint->QuestTemplate || !QuestBlueprint->QuestTemplate->GetQuestStartState())
		{
			return false;
		}

		UEdGraph* Graph = EnsureGraph(
			QuestBlueprint,
			QuestGraphClassPath,
			QuestGraphSchemaClassPath,
			TEXT("Quest Graph"));
		if (!Graph)
		{
			return false;
		}

		const FQuestLayoutData Layout = BuildQuestLayout(QuestBlueprint->QuestTemplate);
		if (Layout.OrderedStates.Num() == 0)
		{
			return false;
		}

		UClass* RootNodeClass = LoadRequiredClass<UClass>(QuestRootNodeClassPath);
		UClass* StateNodeClass = LoadRequiredClass<UClass>(QuestStateNodeClassPath);
		UClass* ActionNodeClass = LoadRequiredClass<UClass>(QuestActionNodeClassPath);
		if (!RootNodeClass || !StateNodeClass || !ActionNodeClass)
		{
			return false;
		}

		TArray<UEdGraphNode*> GraphNodes;
		Graph->GetNodesOfClass<UEdGraphNode>(GraphNodes);

		UEdGraphNode* RootNode = nullptr;
		TArray<UEdGraphNode*> FreeStateNodes;
		TArray<UEdGraphNode*> FreeActionNodes;

		for (UEdGraphNode* Node : GraphNodes)
		{
			if (!Node)
			{
				continue;
			}

			if (Node->IsA(RootNodeClass))
			{
				RootNode = Node;
				continue;
			}

			if (Node->IsA(ActionNodeClass))
			{
				FreeActionNodes.Add(Node);
				continue;
			}

			if (Node->IsA(StateNodeClass))
			{
				FreeStateNodes.Add(Node);
			}
		}

		UQuestState* StartState = QuestBlueprint->QuestTemplate->GetQuestStartState();
		if (!RootNode)
		{
			RootNode = CreateGraphNode(Graph, RootNodeClass, StartState, StateColumnX, 0);
		}
		else
		{
			ConfigureExistingNode(RootNode, StartState, StateColumnX, 0);
		}

		TMap<UQuestState*, UEdGraphNode*> StateToGraphNode;
		StateToGraphNode.Add(StartState, RootNode);

		int32 NextFreeState = 0;
		for (UQuestState* State : Layout.OrderedStates)
		{
			if (!State || State == StartState)
			{
				continue;
			}

			const int32 Row = Layout.StateRows.FindRef(State);
			UEdGraphNode* GraphNode = FreeStateNodes.IsValidIndex(NextFreeState)
				? FreeStateNodes[NextFreeState++]
				: CreateGraphNode(Graph, StateNodeClass, State, StateColumnX, Row * RowSpacingY);

			if (!GraphNode)
			{
				return false;
			}

			ConfigureExistingNode(GraphNode, State, StateColumnX, Row * RowSpacingY);
			StateToGraphNode.Add(State, GraphNode);
		}

		TMap<UQuestBranch*, UEdGraphNode*> BranchToGraphNode;
		int32 NextFreeAction = 0;
		for (UQuestBranch* Branch : QuestBlueprint->QuestTemplate->GetBranches())
		{
			if (!Branch)
			{
				continue;
			}

			const int32 Row = Layout.BranchRows.FindRef(Branch);
			UEdGraphNode* GraphNode = FreeActionNodes.IsValidIndex(NextFreeAction)
				? FreeActionNodes[NextFreeAction++]
				: CreateGraphNode(Graph, ActionNodeClass, Branch, ActionColumnX, Row * RowSpacingY);

			if (!GraphNode)
			{
				return false;
			}

			ConfigureExistingNode(GraphNode, Branch, ActionColumnX, Row * RowSpacingY);
			BranchToGraphNode.Add(Branch, GraphNode);
		}

		BreakAllLinks(RootNode);
		for (TPair<UQuestState*, UEdGraphNode*>& Pair : StateToGraphNode)
		{
			if (Pair.Value != RootNode)
			{
				BreakAllLinks(Pair.Value);
			}
		}
		for (TPair<UQuestBranch*, UEdGraphNode*>& Pair : BranchToGraphNode)
		{
			BreakAllLinks(Pair.Value);
		}

		for (const TPair<UQuestState*, TArray<UQuestBranch*>>& Pair : Layout.StateBranches)
		{
			UQuestState* State = Pair.Key;
			UEdGraphNode* StateNode = StateToGraphNode.FindRef(State);
			if (!StateNode)
			{
				continue;
			}

			int32 BranchIndex = 0;
			for (UQuestBranch* Branch : Pair.Value)
			{
				UEdGraphNode* BranchNode = BranchToGraphNode.FindRef(Branch);
				if (!BranchNode)
				{
					++BranchIndex;
					continue;
				}

				const int32 Row = Layout.StateRows.FindRef(State);
				BranchNode->NodePosY = (Row * RowSpacingY) + (BranchIndex * BranchOffsetY);
				BranchNode->SnapToGrid(16);
				ConnectNodes(Graph, StateNode, BranchNode);

				if (UQuestState* DestinationState = Layout.BranchDestinations.FindRef(Branch))
				{
					if (UEdGraphNode* DestinationNode = StateToGraphNode.FindRef(DestinationState))
					{
						ConnectNodes(Graph, BranchNode, DestinationNode);
					}
				}

				++BranchIndex;
			}
		}

		Graph->NotifyGraphChanged();
		QuestBlueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(QuestBlueprint);
		QuestBlueprint->MarkPackageDirty();
		return true;
	}

	struct FDialogueLayoutData
	{
		TArray<UDialogueNode*> OrderedNodes;
		TMap<UDialogueNode*, int32> NodeRows;
		TMap<UDialogueNode*, TArray<UDialogueNode*>> ChildNodes;
	};

	FDialogueLayoutData BuildDialogueLayout(UDialogue* Dialogue)
	{
		FDialogueLayoutData Layout;
		if (!Dialogue || !Dialogue->RootDialogue)
		{
			return Layout;
		}

		TSet<UDialogueNode*> VisitedNodes;
		TQueue<UDialogueNode*> Queue;
		Queue.Enqueue(Dialogue->RootDialogue);
		VisitedNodes.Add(Dialogue->RootDialogue);

		while (!Queue.IsEmpty())
		{
			UDialogueNode* Node = nullptr;
			Queue.Dequeue(Node);
			if (!Node)
			{
				continue;
			}

			Layout.NodeRows.Add(Node, Layout.OrderedNodes.Num());
			Layout.OrderedNodes.Add(Node);

			TArray<UDialogueNode*> Children;
			for (UDialogueNode_NPC* Child : Node->NPCReplies)
			{
				if (Child)
				{
					Children.Add(Child);
				}
			}
			for (UDialogueNode_Player* Child : Node->PlayerReplies)
			{
				if (Child)
				{
					Children.Add(Child);
				}
			}

			Layout.ChildNodes.Add(Node, Children);

			for (UDialogueNode* Child : Children)
			{
				if (Child && !VisitedNodes.Contains(Child))
				{
					VisitedNodes.Add(Child);
					Queue.Enqueue(Child);
				}
			}
		}

		return Layout;
	}

	bool RebuildDialogueGraphInternal(UDialogueBlueprint* DialogueBlueprint)
	{
		if (!DialogueBlueprint || !DialogueBlueprint->DialogueTemplate || !DialogueBlueprint->DialogueTemplate->RootDialogue)
		{
			return false;
		}

		UEdGraph* Graph = EnsureGraph(
			DialogueBlueprint,
			DialogueGraphClassPath,
			DialogueGraphSchemaClassPath,
			TEXT("Dialogue Graph"));
		if (!Graph)
		{
			return false;
		}

		const FDialogueLayoutData Layout = BuildDialogueLayout(DialogueBlueprint->DialogueTemplate);
		if (Layout.OrderedNodes.Num() == 0)
		{
			return false;
		}

		UClass* RootNodeClass = LoadRequiredClass<UClass>(DialogueRootNodeClassPath);
		UClass* NPCNodeClass = LoadRequiredClass<UClass>(DialogueNPCNodeClassPath);
		UClass* PlayerNodeClass = LoadRequiredClass<UClass>(DialoguePlayerNodeClassPath);
		if (!RootNodeClass || !NPCNodeClass || !PlayerNodeClass)
		{
			return false;
		}

		TArray<UEdGraphNode*> GraphNodes;
		Graph->GetNodesOfClass<UEdGraphNode>(GraphNodes);

		UEdGraphNode* RootNode = nullptr;
		TArray<UEdGraphNode*> FreeNPCNodes;
		TArray<UEdGraphNode*> FreePlayerNodes;

		for (UEdGraphNode* Node : GraphNodes)
		{
			if (!Node)
			{
				continue;
			}

			if (Node->IsA(RootNodeClass))
			{
				RootNode = Node;
				continue;
			}

			if (Node->IsA(PlayerNodeClass))
			{
				FreePlayerNodes.Add(Node);
				continue;
			}

			if (Node->IsA(NPCNodeClass))
			{
				FreeNPCNodes.Add(Node);
			}
		}

		UDialogueNode* RootDialogueNode = DialogueBlueprint->DialogueTemplate->RootDialogue;
		if (!RootNode)
		{
			RootNode = CreateGraphNode(Graph, RootNodeClass, RootDialogueNode, StateColumnX, 0);
		}
		else
		{
			ConfigureExistingNode(RootNode, RootDialogueNode, StateColumnX, 0);
		}

		TMap<UDialogueNode*, UEdGraphNode*> DialogueNodeToGraphNode;
		DialogueNodeToGraphNode.Add(RootDialogueNode, RootNode);

		int32 NextFreeNPC = 0;
		int32 NextFreePlayer = 0;

		for (UDialogueNode* DialogueNode : Layout.OrderedNodes)
		{
			if (!DialogueNode || DialogueNode == RootDialogueNode)
			{
				continue;
			}

			const int32 Row = Layout.NodeRows.FindRef(DialogueNode);
			const bool bIsPlayerNode = DialogueNode->IsA(UDialogueNode_Player::StaticClass());
			TArray<UEdGraphNode*>& Pool = bIsPlayerNode ? FreePlayerNodes : FreeNPCNodes;
			int32& PoolIndex = bIsPlayerNode ? NextFreePlayer : NextFreeNPC;
			UClass* NodeClass = bIsPlayerNode ? PlayerNodeClass : NPCNodeClass;

			UEdGraphNode* GraphNode = Pool.IsValidIndex(PoolIndex)
				? Pool[PoolIndex++]
				: CreateGraphNode(Graph, NodeClass, DialogueNode, bIsPlayerNode ? ActionColumnX : StateColumnX, Row * RowSpacingY);

			if (!GraphNode)
			{
				return false;
			}

			ConfigureExistingNode(GraphNode, DialogueNode, bIsPlayerNode ? ActionColumnX : StateColumnX, Row * RowSpacingY);
			DialogueNodeToGraphNode.Add(DialogueNode, GraphNode);
		}

		for (const TPair<UDialogueNode*, UEdGraphNode*>& Pair : DialogueNodeToGraphNode)
		{
			BreakAllLinks(Pair.Value);
		}

		for (const TPair<UDialogueNode*, TArray<UDialogueNode*>>& Pair : Layout.ChildNodes)
		{
			UEdGraphNode* ParentNode = DialogueNodeToGraphNode.FindRef(Pair.Key);
			if (!ParentNode)
			{
				continue;
			}

			int32 ChildIndex = 0;
			for (UDialogueNode* ChildNode : Pair.Value)
			{
				UEdGraphNode* GraphNode = DialogueNodeToGraphNode.FindRef(ChildNode);
				if (!GraphNode)
				{
					++ChildIndex;
					continue;
				}

				GraphNode->NodePosY = (Layout.NodeRows.FindRef(Pair.Key) * RowSpacingY) + (ChildIndex * BranchOffsetY);
				GraphNode->SnapToGrid(16);
				ConnectNodes(Graph, ParentNode, GraphNode);
				++ChildIndex;
			}
		}

		Graph->NotifyGraphChanged();
		DialogueBlueprint->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(DialogueBlueprint);
		DialogueBlueprint->MarkPackageDirty();
		return true;
	}
}

#endif

bool UPangeaNarrativeGraphRebuilder::RebuildNarrativeGraphForAsset(UObject* Asset)
{
#if WITH_EDITOR
	if (UQuestBlueprint* QuestBlueprint = Cast<UQuestBlueprint>(Asset))
	{
		return RebuildQuestGraphInternal(QuestBlueprint);
	}

	if (UDialogueBlueprint* DialogueBlueprint = Cast<UDialogueBlueprint>(Asset))
	{
		return RebuildDialogueGraphInternal(DialogueBlueprint);
	}
#endif

	return false;
}

bool UPangeaNarrativeGraphRebuilder::RebuildTutorialNarrativeGraphs()
{
#if WITH_EDITOR
	UObject* QuestAsset = StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial.QB_Pangea_Tutorial"));
	UObject* DialogueAsset = StaticLoadObject(UObject::StaticClass(), nullptr, TEXT("/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21.DB_Huscarl_N6-N21"));
	return RebuildNarrativeGraphForAsset(QuestAsset) && RebuildNarrativeGraphForAsset(DialogueAsset);
#else
	return false;
#endif
}
