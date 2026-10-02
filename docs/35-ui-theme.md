# M11 local UI theme

The first foundation increment migrates the survival-status panel and weather
activity badge to `FKalmalaUITheme` shared panel/text styling. Edit
`Config/DefaultKalmalaTheme.ini`, section `[Kalmala.UI.Theme]`, and restart the
game/editor process. Unreal's local `KalmalaTheme` config hierarchy is loaded
once, read-only; widgets share the resolved theme. No live reload is supplied.

| Key | Default | Accepted values |
| --- | --- | --- |
| Panel | (0.025, 0.035, 0.04, 0.94) | Linear RGBA components in 0–1 |
| Heading | (0.75, 0.82, 0.79, 1) | Linear RGBA components in 0–1 |
| Text | (0.93, 0.96, 0.94, 1) | Linear RGBA components in 0–1 |
| HighContrastPanel | (0, 0, 0, 0.98) | RGB always black; alpha at least 0.98 |
| HeadingSize | 10 | 8–32, rounded to nearest integer |
| BodySize | 13 | 8–32, rounded to nearest integer |
| EmphasisSize | 17 | 8–32, rounded to nearest integer |
| PaddingX / PaddingY | 12 / 9 | 0–24 logical units |
| RowSpacing | 5 | 0–16 logical units; survival rows only |

Colours use `(R=...,G=...,B=...,A=...)` syntax. Missing, malformed,
non-finite, and out-of-range values fall back independently to the established
defaults. Text uses Unreal's existing default font. Local 100/125/150% text
scale applies after theme sizes; local high contrast forces white text on a
black panel. The weather badge now uses the same panel padding as survival
status. No input, owner-state query, RPC, timer, authority, or gameplay/save
contract changes.

Verify the foundation with `Kalmala.UI.Theme.LocalPresentation`,
`Kalmala.UI.SurvivalStatus.LocalPresentation`, and
`Kalmala.UI.WeatherActivity.LocalPresentation` using the isolated headless
pattern in `07-development-setup.md`. Theme automation checks shared widget
application, accessibility precedence, malformed values, and missing config.
It does not establish rendered layout or packaged config inclusion.

The first M11 parent remains incomplete. Font assets/weights, outlines,
buttons and interaction states, image references, slots, animation settings,
and migration across inventory, building, map, and options remain in that
same parent. Add those keys only with a real consumer and documented fallback;
do not introduce speculative unused configuration. Full parent integration
and rendered viewport/accessibility checks remain pending.
