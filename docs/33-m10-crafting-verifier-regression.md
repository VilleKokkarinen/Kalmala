# M10 rendered release verification corrections

## Crafting verifier rationale

The clean-profile crafting run exposed a stale development assertion in the
rendered crafting fixture. Its camp-feedback probe still required descriptions
for `SmokeFrame` and `SmokeBoarMeat`, although the frozen schema-4 catalogue
explicitly removes the Smoke Frame and legacy smoke recipes in
`docs/28-m9-camp-equipment-recipes.md` and `docs/31-m10-scope-freeze.md`.
The assertion reported `M9 camp feedback: Passed=0` even while the supported
Chest and direct-construction descriptions matched their current contracts.
The PowerShell runner also waited for an obsolete `M9 camp rejected mutations`
log that the current host/client fixture no longer emits; its current
`Crafting owner final` checks already require rejected requests and unchanged
inventory, while the automation namespace covers cooking-experience rules.

Keep the release baseline unchanged. The fixture now requires both retired
descriptions to resolve as `Unknown recipe`, while retaining its positive Chest
and direct-build feedback checks. This corrects development verification only;
it changes no gameplay, presentation, authority, replication, save, or catalogue
behavior.

## Settings modal rationale

Rendered inspection also found that the Settings widget built its root tree in
`NativeConstruct`, after UUserWidget had already created its Slate wrapper.
Build the tree in `NativeOnInitialized` so the existing open/focus behavior has
a visible modal. The maximum 150% text scale also pushed the audio and settings
labels below the old fixed panel, so the panel now reserves 1000 by 980 logical
units. Its wider body keeps the interaction/combat volume label on two lines at
maximum text scale. The controls page also squeezed its label and keyboard/
controller bindings into one row; each control now stacks those three elements
and the list scrolls inside the remaining panel area. The rendered probe waits
for the tab layout to settle before capturing. Its pixel check requires the
backdrop to dim an area outside the panel. These display fixes preserve
local-only settings input and save behavior; they change no multiplayer
contract.

## Verification

Rebuild `KalmalaEditor Win64 Development` in the disposable project mirror,
then run `Scripts/Verify-Crafting.ps1 -Rendered` with separate temporary host
and client profiles. The catalogue automation continues to verify the retired
recipe IDs are absent. Run `Scripts/Verify-SettingsAccessibility.ps1` in the
same mirror and inspect all six captures; the pixel guard proves the modal
backdrop was actually rendered on host and client for all three tabs.
