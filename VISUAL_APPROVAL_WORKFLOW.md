# TAKS World — owner-controlled visual acceptance

**Mandatory: each visual asset is separately accepted by the owner.**
This is not a blanket approval of an entire landscape, model pack or APK.
Status labels: `pending`, `changes_requested`, `rejected`, `approved`.

1. Stage a proposed new image, mesh, texture, animation or UI graphic in a **review-only** pull request or GitHub Actions screenshot artifact. Show its exact preview to the owner and label the intended in-game use, scale, provenance and version. A bundle of ten images is ten review decisions.
2. Wait for an explicit decision: **Elfogadom / Javítani kell / Elutasítom**. Silence, a previous approval of a different asset, and generating an image never count as acceptance.
3. Only after a clear **Elfogadom** for that exact preview/variant, register the precise runtime file path, `status: approved`, SHA-256 of that file and a verifiable `approval_reference` in `assets/visual_approvals.json`. The manifest is a review ledger, not a substitute for the owner's actual decision.
4. If the image/mesh/texture/animation changes by even one byte, repeat the acceptance; the SHA-256 check blocks a silent replacement. Changing the existing accepted original reference is forbidden.
5. The APK job enumerates `app/src/main/assets/{logo,menu,wonderwerk,models}` and **skips the distributable build** if any visual is unregistered, pending, rejected, modified, or missing approval evidence. CI screenshots and review-only packages remain possible.
6. Models sourced from external repositories additionally need original URL, license, transformation and historically grounded suitability checks. A CC0 license is not user approval.
7. `art/reference_crops/Wonderwerk_etalon.jpg` remains the owner's source/reference image; no script may rewrite the original. Approve every new extraction or rendering separately for integration.
8. Build passing, emulator render, device test and owner acceptance are **different states**. Do not call the current primitive scene a finished TAKS World game.

Run offline: `python -m unittest discover -s tests -p 'test_visual_approvals.py'`.
To check a prepared APK asset tree: `python scripts/check_visual_approvals.py`.
