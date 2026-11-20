# TAKS World 0.16.4 audio test

Base: accepted-menu branch at 3bc921a991afcafa46a5efb87f3417b108d4d46d. No evidence of an accepted 0.17 source was found. Existing menu artwork, overlay and logo files are unchanged.

## Implemented
- Offline, original Java PCM synthesis: rain, wind, moving-water texture, cave drips/echo, fire crackle, vegetation rustle, stylized animal and bird calls.
- Continuous random processes and persistent filters/oscillators; no recorded sample loop or clip seam. Gain changes fade over roughly 190 ms. Cave echo is a feedback delay, not a looped recording.
- JNI snapshot from the same locked World used for physics/rendering. Rain intensity, canonical soil water/mud, wind, remaining pool water, actual live animal positions and fire fuel drive sound.
- Water/fire distance attenuation and world-axis stereo pan; cave depth/shelter reduce exterior sound. Approximate cave zone matches the current renderer geometry.
- Travel-derived footsteps: dry soil, mud, stone, grass, shallow water, snow. No steps while jumping or blocked; load/reset does not emit travel steps.
- Android game audio focus, system media volume, pause/resume and worker shutdown.
- C++ causal tests, Java continuous-buffer/silence/fade/travel tests, APK content/hash checks and signed debug test release.

## Source / rights
Sound synthesis code was written for this change. No external recordings or sound libraries were used; no third-party sound license is asserted. Bird ambience is a weather/daylight estimate, not a simulated bird population. Animal calls are stylized, not verified species recordings. Existing art/model rights are unchanged; see their original manifests.

## Preserved / unavailable
The archive has 16 actual RGBA character PNGs (8 idle + 8 crouch), already integrated as fallback for the 3D player. They are restored byte-for-byte. It does not contain full eight-direction cave, ground, animal, plant or dedicated NPC sets. None were invented.
The approved archive/menu hashes and repository logo WebP bytes are verified. The standalone original PNG with SHA256 7109006c1190b8d00e62656c99616506f7b1068864c181655df99a291fe55d7a was not available; this change cannot certify its pixels against the current menu. No logo is regenerated or edited.

## Limits
Approximate enclosure, fixed-axis stereo, single nearest-combined animal sound texture, no ray-based occlusion/convolution, no NPC footstep emitters. Audio is a functional synthetic first implementation; listening and physical Android-device acceptance are still required. Debug signing cannot guarantee upgrade compatibility with a separately signed earlier APK.
