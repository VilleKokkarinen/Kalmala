# Near-crosshair interaction prompt captures

These captures come from the non-shipping host/client presentation review
fixture. Target names and unavailable reasons are synthetic UI states; live
target selection and server validation are covered separately by the crafting
acceptance run and `Kalmala.Gameplay.Interaction.ServerOnlyRangeValidation`.

| Profile | Resolution | Text scale | Contrast | Capture pairs |
| --- | --- | --- | --- | --- |
| Standard | 1280x720 | 100% | Standard | Host/client available and unavailable |
| High contrast | 1024x768 | 150% | High contrast | Host/client available and unavailable |

Modal and no-target clearing were captured and checked in both peers and both
profiles. The rendered crafting acceptance additionally checked that the live
prompt hides while its modal is open.
