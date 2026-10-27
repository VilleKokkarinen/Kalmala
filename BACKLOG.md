# Development backlog

## Active queue

### Valheim-inspired Kalmala lighting and materials — user request, 2026-10-25

**Goal:** Build an original Kalmala lighting/material stack with readable
day/night scenes, atmospheric distance, weather-aware surfaces, and optional
hardware ray-tracing quality levels. The detailed, researched implementation
contract and acceptance scenes are in the [lighting plan](docs/valheim-like-lighting-plan.md).

**Status:** The L01–L06 implementation and milestone-final verification passed
on 2026-10-27. The RTX 5090 is a reference machine only; no release minimum or
recommended GPU, hardware-vendor floor, or performance threshold is established.
Keep the accepted Kalmala art direction, client-only cosmetic behavior, and
existing weather/gameplay authority. Hardware ray tracing and DLSS remain
optional presentation features; Compatibility retains the RT-off visual
baseline. Default Lit remains the production shading path.

**Worktree sequence:** L01–L06 are ordered, dependent goals. Assign each goal
one agent in a dedicated worktree based on integrated `master` after the preceding
lighting goal has been integrated. Independent Kalmala runs may proceed concurrently
in distinct worktrees under `AGENTS.md`; this lighting sequence remains ordered
because later stages depend on earlier lighting outputs.

**Run rule:** In the first unchecked goal, complete exactly its first unchecked
child in order; split it before coding if it exceeds one turn. Run applicable
lightweight checks, update backlog and progress, commit only run-owned changes,
and stop. L06 is the integrated milestone-final verification and repair goal;
do not mark the lighting milestone complete before it passes.

- [x] **L01 — Establish the lighting baseline and core post-process look.**
  Record repeatable scene captures, stabilize the existing light/sky/fog/exposure
  stack, and use built-in post-process volume controls before custom passes.
  - [x] **L01a — Inspect the active prototype map and renderer.** Record the
    active RHI, GI/reflection method, directional/sky lights, fog, exposure,
    post-process volumes, generated material assignments, and capture settings
    in the existing lighting plan. Select and record fixed camera positions for
    all five baseline scenes. Do not change project behavior in this audit.
    The live map/renderer audit and five fixed camera anchors are recorded in
    `docs/valheim-like-lighting-plan.md`; L01b reviewed the daylight, forest,
    and water images. L01c1 added capture-only condition controls; L01c2
    rendered both conditions and verified the five-scene seed-418 set. The
    plan records the missing rain/flame visuals and unreadable wide fire view.
  - [x] **L01b — Capture seed-418 daylight, forest, and water.** Use the fixed
    camera poses for Meadows daylight, forest shade, and coast/water. Preserve
    captures and manifest under `TestResults/LightingBaseline/`; record
    resolution, exposure, hardware, and renderer settings. Do not write under
    `Saved/`, `Intermediate/`, or `DerivedDataCache/`.
  - [x] **L01c — Capture seed-418 weather and fire scenes.** Capture
    overcast/rain and fire-lit dusk/night using L01a's matching camera and
    exposure records; verify all five seed-418 scenes are present.
    - [x] **L01c1 — Add capture-only condition controls.** Add deterministic
      non-Shipping overcast/rain and fire-lit dusk/night options to the existing
      baseline capture runner; keep default captures and ordinary game sessions
      unchanged.
    - [x] **L01c2 — Render and review the weather and fire frames.** Compiled the
      capture fixture in an isolated short mirror, preserved both fixed views
      and manifest entries, and verified all five seed-418 scenes. Recorded no
      rain particles or modeled flame, the unreadable wide fire-light view, and
      unknown effective EV bounds in the lighting plan.
  - [x] **L01d — Capture seed-419 daylight, forest, and water.** Reused L01a's
    fixed views and the seed-418 manifest for Meadows daylight, forest shade,
    and open water. Recorded that the forest anchor has no visible forest and
    the water frame shows a checkerboard surface; these remain baseline
    limitations, not accepted scene looks.
  - [x] **L01e — Capture seed-419 weather and fire scenes.** Capture the
    matching overcast/rain and fire-lit dusk/night views; verify the full
    two-seed baseline set before changing the rig. Both fixed views passed the
    capture checks and are recorded in the shared manifest; the images retain
    the missing-rain and unreadable-fire baseline limitations.
  - [x] **L01f — Tune the scene-wide rig and grade.** Keep solid surfaces on
    Default Lit. Stabilize exposure, balance the existing directional key and
    sky fill, tune existing fog, then apply a restrained global grade and bloom
    using built-in settings. Save the selected values with before/after captures.
  - [x] **L01g — Add only justified local post-process volumes.** Use bounded
    volumes only for an existing authored location needing a distinct look;
    set explicit priority/blend radius and confirm no visible boundary or UI/text
    treatment. If no such location exists, record this child N/A in the plan.
  - [x] **L01h — Pass the core-lighting gate.** Reviewed all ten rebuilt-source
    views and rendered before/after frames from normal third-person sprint/jump
    input. Both seed-specific dusk scenes show a readable silhouette and warm
    ground light using the ordinary 6 m campfire radius and no exposure boost;
    movement-frame exposure remained stable. The final Editor build, 228-case
    automation queue and ten capture checks passed. Documented pre-existing
    seed-419 forest, water, rain and world-horizon limitations in the lighting
    plan and L01h evidence manifest.

- [x] **L02 — Build and integrate the shared surface-material family.** Replace
  flat generated surface materials through project-owned parents and instances,
  retaining direct light, shadow, indirect fill, water clipping, and runtime
  material assignment behavior.
  - [x] **L02a — Define the shared material interface.** Use the existing
    `Content/Kalmala/World/Materials/` path and the lighting plan's bounded
    controls: base tint, roughness, normal strength, broad color variation, and
    mapping scale. Keep common surface functions in Material Functions. Created
    the Default Lit `M_KalmalaSurfaceBase` parent and the reusable
    `Functions/MF_KalmalaSurfaceResponse` graph with those five artist controls;
    existing runtime material assignments remain unchanged for L02b.
  - [x] **L02b — Integrate terrain and shore.** Route the existing grass/soil/
    rock/snow blend through the shared surface response and assign its terrain
    material instance. Assign the lake-shore instance of the shared lit parent
    with the former base tint and roughness preserved. Confirm procedural
    material references load and the biome-debug override remains after normal
    terrain assignment.
  - [x] **L02c — Integrate rock.** Assign `MI_GeneratedRock`, an instance of
    the shared opaque lit parent, only to generated Meadow rocks. Keep roughness
    at 0.95 and compensate their existing vertex-color mean to retain the prior
    rock palette while exposing its subtle facet variation. Preserve the mesh
    geometry/normals and the existing player/campfire material assignments;
    source and asset checks passed, with rendered comparison remaining for L02g.
  - [x] **L02d — Integrate wood and bark.** Generate `MI_GeneratedBark` and
    `MI_GeneratedWood` from the shared lit parent without runtime graph creation.
    Assign the white-tinted bark instance to generated Meadow trunks and tree
    harvest meshes to preserve their species/cut-face vertex colors; assign the
    brown-tinted wood instance to authored held-tool shafts and club/bow pieces.
    Keep the player head, non-tree gatherables, and non-wood tool heads on the
    legacy material. Build, instance-load and presentation-ownership checks
    passed; rendered response remains for L02g.
  - [x] **L02e — Add the foliage family.** Generate a separate two-sided opaque
    canopy parent and instance; assign it to generated Meadow canopies while
    preserving their vertex-color palette. Source review found no opacity
    cutouts or time-varying material input, so masking is unnecessary and shader
    noise is world-stable. Build, instance-load, and ownership checks passed.
    L02g reviewed the fixed daylight/shade silhouettes and shadows; a short
    16-frame, subpixel TAA pan showed no obvious foliage shimmer.
  - [x] **L02f — Integrate water separately.** Generate and assign `MI_GeneratedWater`
    from the shared lit parent at white tint and roughness 0.28, preserving
    existing ocean and lake vertex-color palettes. Keep sea/lake clipping,
    water levels, and shoreline geometry unchanged. Build, commandlet-load,
    and ownership checks passed. L02g reviewed the open-water and repaired
    seed-418 coast frames; the water reads blue, but reflections/highlights were
    not established at these fixed wide views.
  - [x] **L02g — Pass the material-family gate.** Verify terrain, shore, rock,
    bark, canopy, and water retain distinct values across all baseline scenes;
    record material assignments and remaining limitations in the lighting plan.

- [x] **L03 — Prototype the stylized direct-light response.** Keep Default Lit
  as the shipped default while measuring a restrained banded alternative; this
  goal produces evidence and a recommendation, not an unreviewed global style
  change.
  - [x] **L03a — Create an isolated comparison fixture.** Compare Default Lit
    with a soft two- or three-band direct-light treatment on representative
    ground, rock, wood, canopy, creature, and construction surfaces using L01's
    fixed views.
  - [x] **L03b — Evaluate UE 5.8 Substrate NPR support.** Prototype its
    experimental ramp-based shading only if the project's configured renderer,
    build targets, and materials support it; do not add an engine fork or make
    the experimental feature a production dependency.
  - [x] **L03c — Check lighting integration.** Record the fixed-view captures,
    projected sample temporal crops, and 120-frame per-view GPUTime summaries in
    `TestResults/L03c/` and the lighting plan. The test-only Toon candidate shows
    a dim FireDusk response and reduced sample readability. A ForestShade sun-off
    test with the Engine GrayTextureCube confirms skylight fill reaches Toon, but
    all six projected crops are dimmer; world-bounce/GI remains untested. This
    completes the evidence check, not an adoption pass.
  - [x] **L03d — Record the final review recommendation.** Keep Default Lit
    in production. Do not adopt the test-only Substrate Toon candidate: its
    captured FireDusk and controlled skylight proxy crops were dimmer, while
    whole-view GPUTime showed no consistent material-attributable advantage.
    Production meshes, world-bounce/GI, and supported target platforms remain
    unverified. Any broad visual-identity change still requires explicit review.

- [x] **L04 — Add supported weather and environment effects.** Use only existing
  client presentation signals and effects; keep this cosmetic, gradual, and
  outside gameplay, save, RPC, and replicated-state contracts.
  - [x] **L04a — Map existing weather inputs.** Trace currently available
    client-side precipitation, wetness, snow/biome, sky, fog, and world-space
    effect inputs. Record which plan effects have valid inputs; do not add a new
    weather type or network/save field to supply a missing one.
  - [x] **L04b — Add subtle wet-surface response.** Add up to 8% base-color
    darkening and a 0.12 roughness reduction to generated rock, bark, and held
    wood through the existing replicated precipitation signal and gradual local
    presentation. Other materials opt out; the generic signal cannot distinguish
    rain from snow. See the lighting plan for implementation limits.
  - [x] **L04c — Add eligible snow coverage.** Apply it only to suitable
    surfaces with an existing client snow/biome signal. Mark unsupported
    combinations N/A rather than inventing a signal.
  - [x] **L04d — Add eligible moss/lichen variation.** Apply it only to suitable
    surfaces with an existing client biome/surface signal; record unsupported
    combinations as N/A in the lighting plan.
  - [x] **L04e — Tune existing sky and atmospheric effects.** Adjust available
    cloud, local fog, and weather color treatments through the existing pipeline;
    add no new sky model or weather type.
  - [x] **L04f — Tune existing world-space weather effects.** Adjust available
    rain, snow, mist, or ember particles and keep them out of key sight lines.
    Add no full-screen droplets or overlays.
  - [x] **L04g — Pass the environment-effects gate.** Review daylight, rain,
    overcast, and fire-lit captures and transitions; confirm no popping,
    excessive visual noise, or gameplay/simulation change. See the L04g review
    record in the lighting plan.


- [ ] **L05 — Add scalable hardware ray-tracing quality levels.** Use the RT-off
  baseline, Lumen hardware ray tracing for supported Windows DX12/SM6 devices,
  and the measured levels in the lighting plan. Do not claim a minimum GPU until
  representative hardware has passed.
  - [x] **L05a — Verify the actual renderer support path.** Check project RHI,
    SM6, Lumen and hardware-ray-tracing settings; record the exact supported
    device/OS conditions and keep the current `RTX 5090` performance record
    labelled as the reference system, not a minimum-spec test. UE 5.8's
    Windows Lumen path requires Windows 10 build 1909.1350 or newer, DX12, and
    SM6; the hardware-RT-specific GPU list is NVIDIA RTX 2000-series+ or AMD
    RX 6000-series+. The 2026-10-26 L01a Editor/PIE audit observed D3D12/SM5,
    dynamic GI off, screen-space reflections (SSR), and hardware RT off on the
    RTX 5090. At the time of that audit, tracked settings did not pin those
    observed values; L05c now sets an explicit RT-off Compatibility startup
    profile while compiling optional support. This is dated baseline evidence,
    not a Kalmala release hardware floor; see
    `docs/valheim-like-lighting-plan.md` for source links and audit details.
  - [x] **L05b — Preserve the RT-off compatibility path.** The accepted L01h
    visual baseline used the L01a-observed RT-off setting. Compatibility keeps
    hardware tracing off and uses no dynamic GI plus SSR, even though the
    project now builds optional RT shader support for Balanced. Require project
    permission, Windows D3D12, SM6, and shader-platform/RHI RT support before a
    hardware request resolves; automation covers each missing capability.
  - [x] **L05c — Implement the Balanced tier.** Add a local Graphics-menu
    selection that is offered only on supported adapters. Balanced uses Lumen
    hardware tracing for GI/reflections, Surface Cache lighting, and no Hit
    Lighting; the profile configures those settings separately from
    Compatibility. Rendered comparison remains in the L06c integration gate.
  - [x] **L05d — Implement the High tier selectively.** High raises only Lumen
    GI/reflection scalability to Epic quality and keeps Hit Lighting disabled.
    No ray-traced shadow is enabled without visible-benefit evidence. Generated
    forest trunks/canopies are each one procedural mesh per patch, so component
    culling would remove whole patches; no generated grass/sky or instanced
    repeated-mesh classes were found in source, so those cases are N/A.
  - [ ] **L05e — Measure supported hardware.** Compare RT-off, Balanced, and
    High on the reference PC and the actual lower-tier devices selected for
    release; include AMD hardware if it is within the support target. Record
    frame time, resolution, memory, image quality, and artifacts. Do not infer
    minimum/recommended hardware from UE's compatibility list alone. The user
    selected the RTX 5090 reference PC for the current measurement pass and left
    lower-tier/AMD coverage pending.
    - [x] **L05e1 — Add capture-only quality selection.** Extend the baseline
      capture path to request Compatibility, Balanced, or High through the
      existing local graphics-quality profile, require an explicit success log,
      and label the GPU CSVs by tier. Keep the override gated to non-Shipping
      baseline captures with an isolated user directory; leave normal sessions
      on their saved quality setting.
    - [ ] **L05e2 — Measure integrated reference-PC profiles.** Measure the ten
      seed-418/419 fixed views at Compatibility, Balanced, and High on the RTX
      5090. Record per-view GPU frame time, output resolution, memory, and
      visible/temporal artifacts for L06c. Split the captures into ordered
      tier children and retain evidence under `TestResults/L05e2/`. Lower-tier
      and AMD measurements remain pending hardware selection; do not infer a
      release floor from the RTX 5090 pass.
      - [x] **L05e2a — Capture Compatibility tier.** Capture all ten fixed
        views at 1024x768 with 120 GPUTime samples and eight temporal frames at
        0.5-second intervals. Record RTX 5090 memory telemetry and inspected
        scene/artifact notes. See the L05e2a plan entry and preserved evidence.
      - [x] **L05e2b — Capture Balanced tier.** Repeat all ten fixed views and
        the same GPU-time, temporal, resolution, memory, and visual recording
        for Balanced on the RTX 5090.
      - [x] **L05e2c — Capture High tier.** Repeat all ten fixed views and the
        same GPU-time, temporal, resolution, memory, and visual recording for
        High on the RTX 5090.
  - [x] **L05f — Keep upscaling separate.** Record NVIDIA DLSS as optional and
    separate from ray tracing. Add no vendor plugin/dependency or Frame
    Generation path in this goal; seek a separate user-approved plan if they
    become a release requirement. The project plugin/dependency audit found no
    DLSS, Streamline, Ray Reconstruction, or Frame Generation integration; see
    the dated L05f note in the lighting plan.

- [x] **L06 — Integrate and perform lighting milestone-final verification.**
  This is the only full verification gate for the lighting milestone; complete
  it after L01–L05 are integrated. The integrated final gate passed on
  2026-10-27; see the L06e verification record.
  - [x] **L06a — Inspect the combined implementation.** Reviewed the merged
    material, lighting, environment, and RT changes. No duplicate production
    material assignments or unintended Content assets were found. The two water
    components intentionally share one instance, and the biome debug material is
    an explicit capture override. Production presentation reads existing
    replicated weather and remains local; one non-Shipping capture helper sets
    existing hearth state through server authority. RT quality is a local user
    preference in GameUserSettings.ini, not gameplay SaveGame data. No production
    gameplay, replication schema, RPC, or save behavior was added. Full build and
    rendered acceptance remain for L06b/L06c.
  - [x] **L06b — Run the project-prescribed build and focused checks.** The UE
    5.8 integrated Editor build, focused RT/material/weather automations, and
    Compatibility-tier rendered smoke capture passed in the isolated mirror.
    Exact commands and remaining performance/visual scope are in `PROGRESS.md`.
  - [x] **L06c — Capture integrated visual acceptance.** Reviewed the ten fixed
    views at Compatibility, Balanced, and High and inspected the default
    third-person camera/HUD in PIE. See L06c1/L06c2 for evidence and limitations.
    - [x] **L06c1 — Review cross-tier fixed-view evidence.** Compare the ten
      seed-418/419 Compatibility, Balanced, and High captures and selected
      temporal frames; record accepted evidence, visible tier changes, artifacts,
      and composition limitations in the lighting plan and PROGRESS.md. This
      child covers static fixed-view review only.
    - [x] **L06c2 — Inspect default third-person camera and HUD in PIE.**
      Checked default framing, horizontal mouse-look, and HUD readability in
      the integrated L_Prototype viewport. No static-view exposure, lens, or
      bloom issue was observed. Walking through an exposure/post-process region
      remains an evidence limitation recorded in L06e.
  - [x] **L06d — Review and repair lighting regressions.** Reviewed the
    integrated build, focused checks, profile captures, and L06c findings. No
    confirmed L06 regression required repair; L06e owns milestone-final verification.
  - [x] **L06e — Finalize the handoff.** The UE 5.8.2 Editor and Game builds
    and the full 230-test Kalmala automation queue passed. All 30 fixed-view
    profile captures have valid measurements and per-view effective states;
    Compatibility is 0/0/0 and Balanced/High are 1/1/1. Fixed-camera weather
    onset/release captures showed no obvious pop. Normal walking through a
    forest/post-process region could not be exercised in the prototype viewport
    and remains explicitly unverified. Only the RTX 5090 reference machine was
    tested; no release hardware floor is claimed. See the final plan and
    `PROGRESS.md` record. L06 is complete for this milestone gate.

### Developer console and secret-key debug mode — user request, 2026-10-26

- [ ] **DC — Add an in-game developer console with authenticated debug commands.**
  Provide the practical exploration/testing convenience requested by the user,
  inspired by survival-game consoles, using original Kalmala UI and commands.
  Append this goal after existing active goals; do not change their priority.
  Planning only: all implementation children remain unchecked.

  **Implementation defaults chosen for this plan:**
  - Reuse any existing console/input/settings route; integrate the existing
    Enable Console option candidate instead of creating a second setting.
    Default off, persisted locally; F5 toggles when enabled, Escape closes,
    Enter submits, Up/Down browse 50 session-only history entries, and Tab
    completes permitted command names/catalogue IDs. Retain 200 output lines.
    Capture gameplay input while open, restore prior focus on close, and
    support keyboard/controller focus with a rebindable console action.
  - Public commands: `help [command]`, `clear`, `status`, `pos`, `login`,
    `logout`. `status` shows own debug state/build/world seed; `pos` shows own
    position in metres, biome and heading. Help lists only permitted commands.
    `login` opens a masked secret field, never a secret-bearing command line;
    exclude secrets from history, output, logs, autocomplete and replication.
  - Explicit server opt-in defaults off in solo/listen/dedicated sessions.
    Configure a minimum 32-character random secret through a server-only
    environment/configuration source outside tracked/shipped content; no
    hardcoded/default key and no key/verifier distributed to clients. Missing
    configuration denies login. Rotate via server restart, revoking sessions.
    Choose the existing engine-supported encrypted connection/authentication
    path; reject remote key submission if confidentiality cannot be assured.
    No new online service or dependency is authorized by this goal.
  - Authenticate on the server and bind privilege to the owning connection,
    never a client flag or supplied player ID. Allow at most five attempts per
    connection in a rolling 60 seconds and 30 attempts server-wide per rolling
    60 seconds; excess attempts receive a generic denial. Secret length is
    32–128 characters, comparisons avoid timing-dependent early exit, and
    privilege expires after 30 minutes absolute time, logout, disconnect,
    world/session change or server shutdown. Respawn removes pawn overrides
    but retains an otherwise valid connection authorization.
  - All privileged commands recheck server authorization at execution, with
    at most ten requests/second/connection and a maximum 256-character command
    line. Reject unknown commands/IDs, extra arguments, nonfinite/out-of-range
    numbers and invalid targets without side effects. Use an explicit allowlist,
    never arbitrary Unreal console execution, scripts, file paths or asset loads.
    Audit actor identity, command, sanitized arguments and outcome server-side.
  - Cheats target the caller only unless a command below explicitly addresses
    debug-created entities. Debug privilege/flags, flight/noclip and time/weather/AI
    overrides are transient, never saved. Restore captured values on revocation;
    server restart uses ordinary settings. Reject a global override while held
    by another authorized connection; restore it when its owner loses privilege.
    Item grants/repair use existing persistence transactions and rollback on
    failure; successful changes persist normally. Label this in command help.
    No save-schema changes, item additions, paid services or release changes.

  **Initial privileged command set and exact boundaries:**
  - `tp <x> <y> <z>`: absolute world metres, finite values inside the current
    playable world; require loaded destination, valid capsule clearance and
    server-safe terrain/water placement. Reject unsafe/unloaded sites rather
    than changing streaming budgets. `mark <name>` / `goto <name>`: at most
    ten session-only own-position bookmarks, names 1–24 ASCII letters/digits/
    underscores; replacement allowed. `goto` uses the same teleport validator.
  - `god <on|off>`: bypass incoming health damage/death for the living caller
    only; do not revive, alter attack damage or silently refill stats.
    `heal`: restore own health/stamina to current maxima and clear negative
    status effects using existing server APIs, without changing progression.
    `fly <on|off>`: own server-controlled flight with collision enabled.
    `noclip <on|off>`: enable flight and bypass world/body collision for the
    caller only; keep world bounds and loaded-region limits enforced. Turning
    noclip off retains flight and restores collision only at a clear capsule
    position. Turning flight off also disables noclip and requires safe ground.
    `speed <multiplier>`: own movement multiplier in [0.25,5], default 1.
    Expose separate Fly and Noclip toggles in the authenticated console panel,
    default off and session-only, reflecting server-confirmed state; changes
    use the same command authorization path. No interaction-range bypass.
    Record the last collision-clear, grounded position before either mode.
    On voluntary disable, reject unsafe restoration and retain the current
    mode with feedback. On expiry/logout/disconnect/respawn/world change,
    clear both flags and restore ordinary movement/collision at that safe
    position, or the normal server spawn if it is no longer valid; never
    leave a pawn embedded, falling outside the world or retaining a cheat.
  - `give <itemId> [quantity]`: default 1, integer [1,100], registered ordinary
    inventory/equipment IDs only; respect capacity, instance identity and
    persistence rules, reject the entire grant on failure. Never grant
    buildables or legacy Kit IDs. `repair`: restore own carried equipment to
    its current authored condition maximum, without upgrades or new items.
  - `spawn <creatureId> [count]`: default 1, integer [1,10], registered existing
    creature profiles only, safe server-selected sites within 10 metres of the
    caller and at most 20 live debug creatures per session. Replicate through
    normal creature paths, tag transiently, suppress drops/progression rewards
    and exclude from saves. `despawn [radius]`: default 10 metres, range
    (0,50], removes only debug-created creatures in range, never natural
    creatures, buildings, player inventories or persistent world records.
  - `time <hour|reset>`: hour in [0,24), sets a transient server-owned
    replicated sun-hour override until reset/revocation. Mapping found no
    ordinary runtime day clock; preserve baseline static lighting otherwise. `weather <calm|rain|storm|reset>`:
    select an existing deterministic outcome from the server weather cycle;
    mapping found numeric generated families, not an authored preset registry.
    Reject unsupported names and restore ordinary simulation on reset.
    `ai <pause|resume>`: pause/resume only debug-created creatures; natural
    populations remain unaffected. No new weather, creature or gameplay content.
  - `inspect`: read-only server trace within the ordinary interaction range,
    returning target catalogue/type ID, position and public health/state only;
    no other player's private inventory, save/account identity or entitlement.
    `debug <coords|biome|collision|net|off>`: local diagnostic overlays using
    existing data/debug drawing, no hidden inventory disclosure. `perf`:
    toggle local FPS/frame-time display using existing counters. Require debug
    authorization for these diagnostics too; clear overlays on revocation.
  - `gps <query>` enables one tracked search; `gps off` clears it, and
    `gps` shows the current query/result. Accept quoted multiword names or
    catalogue/type IDs (1-64 characters), case-insensitive; prefer exact IDs,
    otherwise match display-name substrings across existing objects, creatures,
    resources, placed structures and named world features. Show the resolved
    target name and distance in metres. No private inventory/account searches.
    Unknown/no-match queries remain enabled and display a question mark.
  - GPS searches the entire current playable world, not just visible/loaded
    actors. Use authoritative live/persisted entity indexes and deterministic
    world-feature descriptors without spawning actors or loading every region.
    Count only extant entities/features: exclude dead/despawned creatures,
    collected/depleted resources and removed structures; potential creature
    spawn sites do not count as living creatures. Include occluded targets.
    Select the smallest 3D straight-line distance from the caller, with stable
    entity ID as the tie-breaker; no maximum distance inside the world bounds.
    Live transforms override descriptor locations for moving entities.
  - Render an original shaded 3D arrow at the top middle of the gameplay HUD,
    centred horizontally, 64 logical pixels below the safe-area top, within a
    96 x 96 logical-pixel area; reserve space so it does not obscure existing
    HUD controls. Rotate its 3D direction relative to camera yaw/pitch/roll
    towards the selected world position, including behind/above/below targets.
    When within 1 metre inclusive, retain the arrow with an arrived indicator.
    When a completed search proves no extant match, replace the arrow with a
    question mark in exactly the same anchor and label it No match.
  - Re-evaluate nearest-target selection every 1 second and on query changes;
    interpolate orientation each frame with a 0.1-second smoothing time.
    On target removal, stop pointing to stale data and reacquire. Incremental
    indexing/search gets at most 2 ms per server frame across all GPS users;
    retain a Searching state until coverage is complete, never report absent
    merely because an area is unloaded or work is pending. Track world-index
    revisions and discard stale query replies. Return only the selected target
    ID/type/name/position to the authorized owner, at most once per second.
    GPS is debug-authorized, default off, unsaved and cleared on logout/expiry,
    disconnect or world change; respawn rebinds a valid connection's search.
    This grants directional debug discovery only, not permanent map reveal.
  - Arbitrary progression editing, free construction/material-payment bypass,
    map reveal, world reset, natural-entity mass deletion, save editing,
    remote-player control and general server administration are out of scope.
    Console availability must not grant access to existing unrestricted cheats.

  **Run rule:** Complete the first unchecked, unblocked child in order, exactly
  one per run; split further before implementation if it exceeds one turn.
  Perform applicable lightweight checks, update backlog/progress, commit only
  run-owned changes, then stop. Target 10–20 minutes per normal child.

  - [x] **DC01 — Map existing input, settings, server auth and cheat routes.**
    Record concrete reuse points and the confidential login transport in this
    goal; identify and gate any bypass. Keep runtime commands disabled.
    Mapping: connection lifetime is AKalmalaMapPlayerController; attach a
    replicated console component there, with UI in a LocalPlayer subsystem.
    Reuse SettingsWidget local config/action rebinding and normal inventory
    transactions. Remote secret submission requires UNetConnection encryption
    on both endpoints; local authority is confidential without a network hop.
    Disable the native Tilde/four-finger console route; registry never calls
    ConsoleCommand/Exec. Existing map presentation CVars cannot mutate gameplay
    and stay outside this authenticated command dispatcher.
  - [x] **DC02 — Add the local console panel and enable/rebind controls.**
    Verify open/close, input capture, focus restore and bounded output.
  - [x] **DC03 — Add the bounded parser and explicit command registry.**
    Verify argument/number rejection; connect public help/clear/status/pos only.
  - [x] **DC04 — Add server opt-in and server-only secret configuration.**
    Verify missing/invalid configuration fails closed without exposing secrets.
  - [x] **DC05 — Add masked login and confidential server authentication.**
    Verify correct/wrong keys, redaction and remote insecure-transport denial.
  - [x] **DC06 — Add login limits, connection grants and revocation.**
    Verify attempt boundaries, absolute expiry, logout/reconnect and respawn.
  - [x] **DC07 — Connect the privileged dispatch authorization/rate gate.**
    Verify forged requests, ownership, audit redaction and no Unreal bypass.
  - [x] **DC08 — Add filtered history, completion and command feedback.**
    Verify bounds, permissions and exclusion of authentication material.
  - [x] **DC09 — Add safe teleport and bounded session bookmarks.**
    Verify valid travel and unsafe/unloaded/out-of-world rejection.
  - [x] **DC10 — Add caller-only god/heal and damage-path integration.**
    Verify incoming damage, death/status behavior and revocation cleanup.
  - [x] **DC11 — Add flight, noclip and speed controls.**
    - [x] **DC11a — Add collision-respecting flight and speed commands.**
      Verify server movement, multiplier bounds and safe grounded restoration.
    - [x] **DC11b — Add server-authorized noclip movement.** Verify collision
      bypass for the caller only, mode dependencies and world/loaded-area bounds.
    - [x] **DC11c — Add separate Fly/Noclip console settings.** Verify default
      off, server-confirmed UI state, command/toggle parity and denied requests.
    - [x] **DC11d — Cover safe movement restoration and revocation.** Verify
      disable inside terrain, last-safe/spawn fallback, expiry/logout, death/
      respawn, disconnect and world change; both flags remain unsaved.
  - [x] **DC12 — Add catalogue-only transactional item grants.**
    Verify quantity/capacity/ID gates, equipment identity, rollback and Kit denial.
  - [x] **DC13 — Add carried-equipment repair through existing transactions.**
    Verify ownership, authored maxima and persistence-failure rollback.
  - [x] **DC14 — Add bounded transient creature spawning.**
    Verify safe placement, caps, peer visibility, no rewards and save exclusion.
  - [x] **DC15 — Add debug-creature despawn and AI pause/resume.**
    Verify radius bounds, debug ownership tags and natural-population exclusion.
  - [x] **DC16 — Add reversible server time override.**
    Verify hour bounds, single-owner conflict, replication and reset/revocation.
  - [x] **DC17 — Add reversible existing-preset weather override.**
    Verify preset allowlist, single-owner conflict and simulation restoration.
  - [x] **DC18 — Add inspect and local diagnostic/performance displays.**
    Verify range/privacy limits, public data only and revocation cleanup.
  - [x] **DC18a - Add GPS world-target indexing and matching.** Implement
    authoritative live/persisted/descriptor lookup, invalidation and full-world
    coverage within the shared frame budget; verify unloaded features, depleted
    resources and actual living creatures without materializing new actors.
  - [x] **DC18b - Add GPS command, nearest selection and owner-only replies.**
    Verify names/IDs, 3D distances/ties, moving/deleted targets, stale results,
    incremental Searching/No match distinction and authorization cleanup.
  - [x] **DC18c - Add the top-centre 3D GPS arrow and question-mark state.**
    Verify camera-relative front/back/vertical directions, nearest changes,
    arrival, no-match replacement, UI scaling and existing HUD clearance.
  - [x] **DC19 — Add focused authorization/lifecycle regression coverage.**
    Cover direct RPC bypass, concurrent authorized users, secret redaction,
    logout/expiry/disconnect, respawn, flight/noclip cleanup and debug-entity
    shutdown cleanup.
  - [x] **DC20 — Document command help and operator setup.**
    Update existing architecture/setup docs with actual configuration, transport,
    persistence consequences and reset behavior; never include a working key.
  - [ ] **DC21 — Perform console milestone-final verification and repairs.**
    Inspect the combined feature; run applicable full build/automation and
    solo/listen/dedicated peer checks from `docs/07-development-setup.md`,
    including unauthorized and authorized remote peers, unsafe input, restart,
    ordinary gameplay after logout, flight/noclip peer movement and safe
    collision restoration, and secret-free logs/history. Verify that
    GPS finds the closest extant match across loaded/unloaded world areas,
    the arrow follows camera/target movement, and a completed absent search
    shows a question mark. Verify build configurations fail closed without
    opt-in, commands affect only permitted targets, normal construction still pays materials, and no debug creatures
    or privileges survive restart. Repair milestone defects and rerun required
    verification; record actual commands/results/limits in `PROGRESS.md`.
    Mark the parent complete only after required verification passes.

## Completed roadmap

| Milestone | Completed scope / acceptance |
| --- | --- |
| M0 — Bootstrap | Project setup, prototype map, packaged build and dedicated-server availability recorded. |
| M1 — Networked traversal and interaction | Replicated movement and server-validated interaction. |
| World generation, Phases 1–9 | Deterministic generation, survival, biome expansion, ocean travel and expanded maps. |
| M2 — Survival camp loop | Gathering, crafting, construction and camp persistence. |
| M3 — Elemental world prototype | Server-owned weather, wetness, exposure and recovery. |
| M4 — Combat and support magic | Combat, creature encounters, discoveries and support effects. |
| M5 — Vertical-slice finish | Complete under recorded acceptance scope. |
| M6 — Production hardening | Complete under recorded supported-session scope and limits. |
| M7 — Survival progression and gameplay UI | Complete. |
| M8 — Ocean and long-distance travel | Complete; dry-shore acceptance waived. |
| M9 — Expanded biome content | Complete under recorded scope. |
| M10 — Release completion and launch validation | Complete under recorded scope and limits. |
| M11 — User experience and visual UI | Final acceptance passed 2026-10-06. |
| M12 — Inventory menu and cleaner HUD | Final acceptance passed 2026-10-09. |
| M13 — Revalidated UI on main | Integrated-main acceptance passed 2026-10-11: 122/122 tests, rendered peers, input/privacy/reconnect and Development/Shipping smoke. [Evidence](docs/evidence/m13/README.md). |
| M14 — Expanded world map | Integrated-main acceptance passed 2026-10-12: editor build, 123/123 tests, live map runners, rendered/performance matrix. [Evidence](docs/evidence/m14/main-integration.md). |

## Completed post-roadmap work

## Scope and history

The [roadmap](docs/04-roadmap.md) and [feature contracts](docs/game-features.md)
remain authoritative for scope; the [verification index](docs/07-development-setup.md#consolidated-verification-index)
records project checks. Detailed implementation history and retired checklists
remain in [PROGRESS.md](PROGRESS.md) and Git history. No acceptance status or
product scope changes as part of this cleanup.
