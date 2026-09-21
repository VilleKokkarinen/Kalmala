# M5 original audio cue contract

This document defines the original audio layer for the existing vertical slice.
It is a cue contract, not an imported sound library or a new gameplay system.
All future sound assets must be project-owned and every cue must have a readable
non-audio equivalent so silence never hides authoritative state.

## Cue matrix

| Cue group | Normal trigger | Audio intent | Required non-audio equivalent | Authority/privacy boundary |
| --- | --- | --- | --- | --- |
| Ambient wilderness | Local player is in normal play | Quiet wind, water, fire, and biome atmosphere establish place without a fixed route | Terrain, weather, hearth, and exposure text/shape cues remain readable | Only local visible context; no hidden population, discovery, or route hint |
| Movement and traversal | Local movement, jump, sprint, landing, or water entry is already visible | Sparse original footfall, cloth, landing, and movement-state cues reinforce action | Existing movement pose, HUD state, and bound input labels remain sufficient | Local cosmetic response; no movement result, speed, stamina, or terrain authority is authored by audio |
| Weather and exposure | Existing replicated weather/Wet presentation changes | Rain, wind, and shelter contrast communicates changing conditions | Wet duration, shelter, warmth, hearth, and recovery text/shape cues remain visible | Server owns weather/exposure/Wet; audio reads accepted replicated state only |
| Interaction and gathering | A normal interaction receives an accepted or rejected result | Short, distinct confirmation or rejection cue avoids ambiguous input | Existing interaction text and inventory result identify accepted, rejected, or unavailable state | Server validates actor, range, payment, item, quantity, and outcome; audio carries no request payload |
| Combat | Replicated committed action or feedback changes for the owning player/relevant actor | Windup, recovery, hit, defeat, and unavailable cues clarify timing and outcome | Combat phase and colour-independent HIT/DEFEAT/UNAVAILABLE text remain authoritative presentation | Server selects target, damage, cooldown, and defeat; clients never infer or author them from sound |
| Discovery | Owner receives a server-confirmed landmark or scroll feedback result | Brief discovery motif acknowledges only an accepted discovery | Owner-only text names the accepted discovery category; remote players retain relevant visible actor presentation | No sound or prompt reveals undiscovered IDs, private rewards, or duplicate state |
| Support magic | Entitled player receives accepted activation, rejection, or expiry presentation | Gentle effect-specific cue communicates support activation and end state | Existing learned-effect, target-validity, cooldown, stamina, and active-state text remains available | Server validates entitlement, effect, target, sequence, stamina, duration, and non-damaging execution |
| Camp and storage | Hearth, construction, or storage result is already visible | Fire, placement, payment, and storage cues reinforce a chosen camp action | Hearth, construction health/rain wear, inventory, and storage text remain readable | Server owns placement, payment, fuel, health, storage, and persistence; audio is presentation only |

The matrix is intentionally event-driven. No cue may start because a hidden
actor, hidden reward, private pin, or future route exists outside the player's
normal visible/relevant context. Repeated state changes should be coalesced or
rate-limited locally so audio cannot become a timing advantage or a source of
network traffic.

## Original asset and mix requirements

Sound assets must be created for Kalmala and stored under a project-owned audio
content path when the runtime pass begins. Do not copy music, field recordings,
voice, sound effects, or named audio identity from a third party. Use the local
Audio settings contract for master/music/ambient/interaction-combat levels and
mute/restore. A muted or unavailable device must still expose all state through
the text/shape equivalents above.

The runtime ambient layer includes loop-seamed, project-generated beds at
`/Game/Kalmala/Audio/WindBed`, `/Game/Kalmala/Audio/WaterBed`,
`/Game/Kalmala/Audio/FireBed`, `/Game/Kalmala/Audio/BiomeBed`, and
`/Game/Kalmala/Audio/RainBed`, plus the one-shot
`/Game/Kalmala/Audio/WetStatusCue` and
`/Game/Kalmala/Audio/SupportAcceptedCue`.
`UKalmalaAmbientAudioSubsystem` starts wind only
for a local player whose normal generated-world state and pawn are ready. It
smoothly adjusts the wind bed from accepted replicated
`FKalmalaWeatherState::WindStrength`. Rain starts only when accepted replicated
precipitation reaches `0.01`, follows its intensity with a quiet local fade,
and stops when rain ends. WetStatusCue plays only when the owning pawn's
replicated server-owned `State.Wet` entry first appears (or is already present
when local play starts); it does not infer the cause or change the status.
Neither cue reads another pawn's private status or sends a request.
SupportAcceptedCue plays once when the owning pawn's existing owner-only
support `FeedbackSerial` advances with `EKalmalaSupportFeedback::Accepted`.
It reports the server-confirmed activation generically and does not infer an
effect from client input or effect state. Existing learned-effect, cooldown,
stamina, and active-state text remains the readable result; unavailable
feedback does not play this acceptance cue.
It samples a bounded set of points around that pawn from the existing immutable
world identity, accepts only the existing sea surface or visible inland-lake
surface, and requires an unobstructed visibility trace from the local view
before water can be heard. The water loop probes every 0.75 seconds, fades by
distance within 1,600 cm, and uses a maximum volume of 0.07. Fire probes locally
replicated campfire actors every 0.75 seconds and accepts only a lit hearth
within 1,400 cm with an unobstructed trace from the owning player's view. Its
non-spatial bed fades with distance, reaches full volume by 275 cm, and caps at
0.075. Biome ambience samples only the owning pawn's current XY from the
locally available immutable seed and existing four-field classifier every
0.75 seconds. One quiet bed uses a small biome-specific pitch/level profile
with smoothed transitions; it does not scan ahead, reveal landmarks, or imply a
route. All five local loops stop when local play ends; no hidden population,
discovery, or remote-biome query is used.

`Scripts/Generate-WildernessWind.ps1`, `Scripts/Generate-WaterAmbience.ps1`,
`Scripts/Generate-FireAmbience.ps1`, `Scripts/Generate-BiomeAmbience.ps1`,
`Scripts/Generate-WeatherExposureAudio.ps1`, and
`Scripts/Generate-SupportFeedbackAudio.ps1` recreate the original mono sources
under `Content/Kalmala/Audio/Source`; the rain bed is seam-crossfaded and the
WetStatusCue and SupportAcceptedCue are short one-shots. Import all seven to
`/Game/Kalmala/Audio` with Unreal's `ImportAssets` commandlet, then run
`Scripts/Verify-AmbientAudio.ps1`, `Scripts/Verify-AmbientAudioPeers.ps1`, and
the forced editor build. Pass `-WeatherExposureOnly` to the peer runner when
focusing on the weather and Wet cues; its default mode also requires visible
water activation. The first verifier checks source format, imported
assets, local-player ownership, context gates, and teardown. The peer runner
confirms independent host/client probes for visible water and hearth context
plus each local player's current sampled biome and accepted rainy weather/Wet
state. Its development-only listen-server fixture selects a light rainy/windy
interval below the hearth smoulder threshold, applies Wet on the server, and
creates a visible server-lit hearth. A test-only server exposure hook keeps
Wet observable while it checks the status cue alongside the hearth bed.
Existing Wet duration, shelter, warmth, hearth, and recovery text remains the
readable fallback. These checks do not establish
audible quality, hardware mixing, or packaged playback, and do not complete
other event cues or audio options.

Keep cues short, bounded, and non-blocking. Ambient layers may be local and
spatial, but they must not stream a second world simulation or reveal a server
actor outside ordinary relevant replication. Combat, discovery, support, hearth,
inventory, and construction cues should follow the same replicated feedback
serials already used by their presentation rather than adding an RPC or save
field.

## No-build audit and runtime limits

`Scripts/Verify-AudioCueContract.ps1` checks all eight cue rows, the
non-audio/accessibility requirement, project-owned asset rule, local mix scope,
server authority/privacy boundary, and the absence of a new RPC or save field.
It does not create sound assets, prove mixing, test spatialization, or launch
Unreal. The wind-bed verifier also does not prove audible playback, packaged
mixing, spatialization, or the remaining cue groups; those remain queued for
the Unreal-verified M5 presentation pass.

## Multiplayer and persistence boundary

Audio is local presentation. The server remains authoritative for world
identity, terrain, population, weather, exposure, interactions, inventory,
construction, combat, discoveries, learned effects, rewards, and saves. A cue
cannot select a target, damage an actor, grant an item, reveal private state,
change a timer, or mutate persistence. Audio settings and cue history are local
preferences and must not be replicated or added to gameplay save schemas.
