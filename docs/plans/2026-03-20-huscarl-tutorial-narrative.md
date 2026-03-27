# Huscarl Tutorial Narrative Implementation Plan

> **For Claude:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task.

**Goal:** Build the Huscarl tutorial flow so interacting with Huscarl starts a free-movement opening dialogue, transitions into a Narrative Tales quest that advances from player input, then ends with a free-movement closing dialogue.

**Architecture:** Keep the flow split across two dialogue assets and one quest asset. `Pangea_NPC_Huscarl` starts `DB_Huscarl_CS1`, the opening dialogue starts `QB_Pangea_Tutorial`, the player controller reports tutorial inputs through the Narrative component with `Complete Narrative Data Task`, the quest advances through built-in dialogue tasks plus data tasks, and the final quest state starts `DB_Huscarl_N6-N21`.

**Tech Stack:** Unreal Engine 5 Blueprint assets, Narrative Tales / Narrative Pro, ACF input events in `BP_Pangea_PlayerController`, Unreal Editor MCP asset/graph tools, Blueprint compile verification.

---

### Task 1: Capture Baseline And Lock The Input Vocabulary

**Files:**
- Modify: `docs/plans/2026-03-20-huscarl-tutorial-narrative-design.md`
- Inspect: `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`
- Inspect: `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`

**Step 1: Write the failing test**

Document the exact task tokens and expected quest order in the design doc before editing assets. Add a small appendix or note that lists the final agreed tokens:

- `Move`
- `Sprint`
- `Roll`
- `OpenInventory`
- `Attack`
- `MultiAttack`
- `Crouch`

Also note that the Huscarl re-engage step should prefer a built-in Narrative talk/dialogue task rather than a synthetic token when possible.

**Step 2: Run test to verify it fails**

Run a baseline inspection of the player controller and quest:

- Read `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`
- Read `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`

Expected: there is no explicit, centralized list of tutorial task tokens and the quest is still only a partial skeleton.

**Step 3: Write minimal implementation**

Update the design doc so the task vocabulary is explicit and final before touching any Blueprint logic.

**Step 4: Run test to verify it passes**

Re-open the design doc and confirm it contains the final token list and the built-in-task note for the Huscarl interaction beat.

Expected: a future implementer can wire the controller and quest without inventing new argument names.

**Step 5: Commit**

```bash
git add docs/plans/2026-03-20-huscarl-tutorial-narrative-design.md
git commit -m "docs: finalize tutorial task vocabulary"
```

### Task 2: Wire Tutorial Input Reporting In The Player Controller

**Files:**
- Modify: `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`

**Step 1: Write the failing test**

Use graph inspection to prove the required reporting hooks are missing. Check for:

- `Complete Narrative Data Task` nodes in the EventGraph
- input events for attack, sprint, crouch, and inventory

Expected: movement and roll delegates already exist, but the full tutorial input-reporting chain is not present.

**Step 2: Run test to verify it fails**

Inspect the EventGraph and compile the controller.

Expected:

- compile succeeds before changes
- there are no quest-reporting calls for the tutorial inputs yet

**Step 3: Write minimal implementation**

Edit `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`:

- keep the existing `ACFMovementInput -> Started` path and use it to report `Move`
- keep the existing `ACFDodgeInput -> Started` path and use it to report `Roll`
- add the relevant `ACF...Input` or enhanced input events for:
  - sprint
  - inventory
  - attack
  - crouch
- on each `Started` pin, call `Complete Narrative Data Task` on `Narrative Tales Component`
- use the exact string arguments from Task 1
- keep the logic flat and local in the EventGraph; do not add a general event bus unless the graph becomes unreadable

For `MultiAttack`, do not add a separate input event. Reuse the attack input and let the quest require quantity `3`.

**Step 4: Run test to verify it passes**

Compile `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`.

Then inspect the EventGraph again.

Expected:

- compile succeeds with zero errors and zero warnings
- `Complete Narrative Data Task` nodes exist for each tutorial input
- each node targets `Narrative Tales Component`
- argument strings match the agreed vocabulary exactly

**Step 5: Commit**

```bash
git add Content/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController.uasset
git commit -m "feat: report tutorial inputs to narrative"
```

### Task 3: Build The Ordered Tutorial Quest

**Files:**
- Modify: `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`

**Step 1: Write the failing test**

Inspect the current quest graph and list what is missing compared with the approved flow:

- full ordered objective chain
- explicit movement, sprint, roll, inventory, attack, multiattack, and crouch tasks
- a built-in Huscarl talk/dialogue task for the weapon handoff beat
- final launch into the closing dialogue

Expected: the graph has only a short skeleton and does not yet represent the full sequence.

**Step 2: Run test to verify it fails**

Compile the quest blueprint and read the quest graph.

Expected:

- compile succeeds before changes
- objective coverage is incomplete
- there is no end-to-end path from tutorial start to closing dialogue

**Step 3: Write minimal implementation**

Edit `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial` so it becomes a linear tutorial quest:

- start state / branch after `CS1`
- task: `Move`
- task: `Sprint`
- task: `Roll`
- built-in Narrative talk/dialogue task for the Huscarl weapon interaction beat
- task: `OpenInventory`
- task: `Attack`
- task: `MultiAttack` with quantity `3`
- task: `Crouch`
- final state that launches `DB_Huscarl_N6-N21`

Implementation rules:

- use built-in Narrative quest tasks for the Huscarl interaction and any dialogue handoff beats
- use data tasks only for input-driven objectives
- keep the quest linear and explicit; do not add optional branches or reusable tutorial frameworks
- ensure the quest descriptions match the intended player instructions from the reference flow

**Step 4: Run test to verify it passes**

Compile `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial` and inspect the quest graph.

Expected:

- compile succeeds with zero errors and zero warnings
- the graph contains the full ordered sequence
- the Huscarl weapon step uses a built-in Narrative task, not a synthetic data token, unless the asset API forces otherwise
- `MultiAttack` requires quantity `3`
- the final node chain starts the closing dialogue asset

**Step 5: Commit**

```bash
git add Content/_Game/Narrative/Quests/QB_Pangea_Tutorial.uasset
git commit -m "feat: build huscarl tutorial quest flow"
```

### Task 4: Finalize Dialogue Handoffs

**Files:**
- Modify: `/Game/_Game/Narrative/Dialogue/DB_Huscarl_CS1`
- Modify: `/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21`
- Modify: `/Game/_Game/Characters/NPC/Pangea_NPC_Huscarl`

**Step 1: Write the failing test**

Inspect the two dialogue graphs and the Huscarl NPC blueprint.

Expected:

- `Pangea_NPC_Huscarl` already starts `DB_Huscarl_CS1`
- `DB_Huscarl_CS1` does not yet fully hand off into the quest
- `DB_Huscarl_N6-N21` does not yet fully represent the closing sequence

**Step 2: Run test to verify it fails**

Compile all three blueprints before changes.

Expected: they compile, but the full start-to-quest-to-finish handoff is incomplete.

**Step 3: Write minimal implementation**

Edit the assets so the handoffs are explicit:

- `Pangea_NPC_Huscarl`
  - preserve the current `BeginDialogue` call to `DB_Huscarl_CS1`
  - keep `bFreeMovement=True`
- `DB_Huscarl_CS1`
  - finalize the opening dialogue text and branch structure
  - end by starting `QB_Pangea_Tutorial`
- `DB_Huscarl_N6-N21`
  - finalize the closing Huscarl lines from the approved flow
  - keep it as a separate free-movement dialogue asset

If the quest can start or finish the dialogues directly, prefer that over putting extra NPC branching logic into `Pangea_NPC_Huscarl`.

**Step 4: Run test to verify it passes**

Compile:

- `/Game/_Game/Characters/NPC/Pangea_NPC_Huscarl`
- `/Game/_Game/Narrative/Dialogue/DB_Huscarl_CS1`
- `/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21`

Inspect the graphs afterward.

Expected:

- all three assets compile cleanly
- Huscarl still begins the opening dialogue on interact
- the opening dialogue launches the quest
- the closing dialogue remains separate from the opening asset

**Step 5: Commit**

```bash
git add Content/_Game/Characters/NPC/Pangea_NPC_Huscarl.uasset Content/_Game/Narrative/Dialogue/DB_Huscarl_CS1.uasset Content/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21.uasset
git commit -m "feat: connect huscarl tutorial dialogue handoffs"
```

### Task 5: End-To-End Blueprint Verification

**Files:**
- Verify: `/Game/_Game/Characters/NPC/Pangea_NPC_Huscarl`
- Verify: `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`
- Verify: `/Game/_Game/Narrative/Dialogue/DB_Huscarl_CS1`
- Verify: `/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21`
- Verify: `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`

**Step 1: Write the failing test**

Create a manual verification checklist for the exact tutorial sequence:

1. Interact with Huscarl
2. Finish `CS1`
3. Trigger `Move`
4. Trigger `Sprint`
5. Trigger `Roll`
6. Re-engage with Huscarl for weapon handoff
7. Trigger inventory input
8. Trigger attack once
9. Trigger attack three times for multiattack
10. Trigger crouch
11. Confirm `CS2` starts

**Step 2: Run test to verify it fails**

Run baseline compile checks on all modified assets and inspect the quest/dialogue graphs.

Expected: before final cleanup, at least one missing handoff, bad token, or compile issue is likely to surface.

**Step 3: Write minimal implementation**

Fix only the issues found during end-to-end verification:

- mismatched token strings
- wrong target component on `Complete Narrative Data Task`
- broken quest links
- incorrect dialogue launches
- compile warnings

Do not refactor unrelated Blueprint logic.

**Step 4: Run test to verify it passes**

Compile all modified blueprints again and perform the manual checklist in editor or PIE.

Expected:

- all modified blueprints compile with zero errors and zero warnings
- the quest advances in the intended order
- the Huscarl interaction beat is handled by a built-in Narrative task if available
- `MultiAttack` only completes after three attack completions
- finishing the crouch step launches `DB_Huscarl_N6-N21`

**Step 5: Commit**

```bash
git add Content/_Game/Characters/NPC/Pangea_NPC_Huscarl.uasset Content/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController.uasset Content/_Game/Narrative/Dialogue/DB_Huscarl_CS1.uasset Content/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21.uasset Content/_Game/Narrative/Quests/QB_Pangea_Tutorial.uasset
git commit -m "test: verify huscarl tutorial narrative flow"
```
