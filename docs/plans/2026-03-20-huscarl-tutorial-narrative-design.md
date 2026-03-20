# Huscarl Tutorial Narrative Design

**Date:** 2026-03-20

**Goal:** Implement the Huscarl onboarding sequence in Narrative Tales using two free-movement dialogue assets around a quest-driven tutorial that advances from player inputs.

## Context

The project already contains the core assets for this flow:

- `/Game/_Game/Characters/NPC/Pangea_NPC_Huscarl`
- `/Game/_Game/Narrative/Dialogue/DB_Huscarl_CS1`
- `/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21`
- `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`
- `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`

`Pangea_NPC_Huscarl` already starts `DB_Huscarl_CS1` with `bFreeMovement=True`. That matters because free movement is configured per dialogue asset, so the opening and closing conversations must stay split into separate dialogue blueprints.

## Approved Approach

Use a quest-owned tutorial flow with direct player-controller input hooks.

- `Pangea_NPC_Huscarl` remains the single entry point.
- `DB_Huscarl_CS1` handles the opening branch and hands off into `QB_Pangea_Tutorial`.
- `QB_Pangea_Tutorial` owns the ordered gameplay tutorial objectives.
- `DB_Huscarl_N6-N21` handles the closing Huscarl dialogue after the final objective.
- `BP_Pangea_PlayerController` reports tutorial actions into the Narrative Tales component by calling `Complete Narrative Data Task` from the relevant `ACF...Input` events.

## Flow Ownership

### Dialogue Assets

`DB_Huscarl_CS1`

- Opening interaction with Huscarl
- Player choice branch:
  - "I have no memory of anything."
  - "I'm fine. Tell me what we need."
- Shared NPC follow-up
- Handoff into the tutorial quest

`DB_Huscarl_N6-N21`

- Closing free-movement dialogue after the tutorial objectives complete
- Contains the post-training Huscarl lines and wraps the sequence cleanly without mixing dialogue movement modes

### Quest Asset

`QB_Pangea_Tutorial`

- Owns the action sequence between the two dialogue assets
- Displays the step-by-step instructions
- Advances only when the matching Narrative task is completed
- Starts the final dialogue asset when the last objective completes

### Player Controller

`BP_Pangea_PlayerController`

- Stays as the source of player input events
- Uses the existing `Narrative Tales Component`
- Calls `Complete Narrative Data Task` on the relevant `Started` pins for the tutorial inputs
- Keeps the implementation simple and local instead of introducing a separate abstraction layer

## Task Strategy

Use built-in Narrative quest tasks where they fit the authored interaction flow, and use Narrative data tasks for pure input-driven objectives.

### Built-In Narrative Tasks

Use built-in quest tasks for dialogue-driven or conversation handoff beats, such as:

- starting or finishing a dialogue when the quest needs to hand off between quest and conversation
- any explicit "talk to Huscarl" style step if it is cleaner to model as a built-in Narrative task than as a synthetic input event

### Narrative Data Tasks

Use `Complete Narrative Data Task` for the input-driven objectives:

- `Move`
- `Sprint`
- `Roll`
- `OpenInventory`
- `Attack`
- `MultiAttack`
- `Crouch`

If the mid-quest weapon handoff is easiest to treat as a direct interaction beat with Huscarl, prefer a built-in Narrative quest task over a synthetic input token. If it stays purely input-driven, use a single stable argument such as `InteractHuscarl`.

## Approved Objective Model

The tutorial should stay input-driven rather than state-verified.

Expected objective sequence:

1. Move
2. Sprint
3. Roll
4. Re-engage with Huscarl to retrieve the weapon
5. Open inventory
6. Perform an attack
7. Perform a multiattack by pressing attack three times
8. Crouch

Recommended mapping:

- `Move` -> one completion from movement input
- `Sprint` -> one completion from sprint input
- `Roll` -> one completion from dodge / roll input
- `Retrieve weapon` -> built-in Narrative talk / dialogue task if practical, otherwise one input-driven interaction completion
- `OpenInventory` -> one completion from the inventory input
- `Attack` -> one completion from attack input
- `MultiAttack` -> quantity `3` from attack input
- `Crouch` -> one completion from crouch input

## Asset Change Plan

### `/Game/_Game/Characters/NPC/Pangea_NPC_Huscarl`

- Keep the existing interact-to-begin-dialogue behavior
- Preserve `DB_Huscarl_CS1` as a free-movement dialogue entry point
- Add or confirm the handoff back into the closing dialogue after the quest finishes if the quest does not own that launch directly

### `/Game/_Game/Narrative/Dialogue/DB_Huscarl_CS1`

- Finalize the opening branch and shared follow-up
- Launch `QB_Pangea_Tutorial` at the end of the opening dialogue

### `/Game/_Game/Narrative/Quests/QB_Pangea_Tutorial`

- Convert the existing skeleton into the full ordered tutorial sequence
- Use built-in Narrative tasks for dialogue/talk handoffs where appropriate
- Use data-task-driven objectives for input actions
- Launch `DB_Huscarl_N6-N21` when the final objective completes

### `/Game/_Game/Narrative/Dialogue/DB_Huscarl_N6-N21`

- Finalize the post-training Huscarl lines
- Keep the asset in free-movement mode to preserve the split-dialogue requirement

### `/Game/_Game/Characters/Player/Blueprints/BP_Pangea_PlayerController`

- Reuse the existing `ACFMovementInput` and `ACFDodgeInput`
- Add the other required `ACF...Input` or relevant enhanced input events
- On `Started`, call `Complete Narrative Data Task` on the `Narrative Tales Component`
- Use stable string arguments for the tutorial tasks so the quest and controller stay aligned

## Verification

Practical verification should cover:

- compile every modified blueprint with no errors or warnings
- confirm Huscarl interaction still begins `DB_Huscarl_CS1`
- confirm the opening dialogue starts the tutorial quest
- confirm each input objective advances the quest in order
- confirm the quest launches `DB_Huscarl_N6-N21` after the last objective
- confirm the dialogue assets remain split for free-movement behavior

## Notes

- Keep the tutorial implementation small and explicit. Do not add a general-purpose tutorial framework.
- Prefer one clear task argument per tutorial action.
- Use built-in Narrative tasks whenever they better represent a dialogue or talk-to-NPC beat than a synthetic input completion.
