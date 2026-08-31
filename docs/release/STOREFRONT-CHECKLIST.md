# DATA storefront release checklist

## Release strategy

Ship the first public Windows release candidate on itch.io. Start Steamworks onboarding and publish the Steam Coming Soon page in parallel, then promote a tested Windows candidate to Steam.

## Engineering gate

- [x] Release configuration builds on Windows x64.
- [x] Package contains the executable and all runtime asset directories.
- [x] Package contains build provenance, per-file hashes, notices, and dependency licenses.
- [x] Packaged executable passes `--smoke-test`.
- [x] Packaged executable passes `--model-test` from outside the repository.
- [x] Native gameplay suite passes.
- [ ] Human-play the extracted package for at least 30 minutes on the release machine.
- [ ] Test the extracted package on a second Windows 10 or 11 computer with no developer tools installed.
- [ ] Verify keyboard/mouse and one common controller from the packaged build.
- [ ] Verify progression survives restart and identify its support/reset location for players.
- [ ] Verify online play against the production multiplayer endpoint, or label online play unavailable for RC1.
- [ ] Scan the final archive with Windows Security and VirusTotal; retain the result URLs.
- [ ] Decide the public version and replace `0.1.0-rc.1` if necessary.
- [ ] Merge the intended release commit to `main` only after reviewing the three pre-existing untracked files.

## Rights and policy gate

- [x] Bundle known desktop dependency and font licenses.
- [ ] Confirm ownership or commercial distribution permission for every audio file, model source, GIF-derived asset, logo, and promotional image.
- [ ] Choose the public publisher/developer name and copyright line.
- [ ] Publish a support contact.
- [ ] Audit what the multiplayer service receives or stores, then publish an accurate privacy statement before enabling online play publicly.
- [ ] Decide whether RC1 is free, pay-what-you-want, or paid.
- [ ] Confirm content disclosures and age-rating answers for each storefront.

## itch.io gate

- [ ] Create the itch.io project page and reserve the final URL slug.
- [x] Generate and visually inspect five genuine 1280×720 gameplay screenshots.
- [ ] Create a 630×500 cover image and select the final 3–5 screenshots.
- [ ] Complete title, short description, description, controls, platform, status, genre, tags, and pricing.
- [ ] Upload the complete Windows ZIP through Butler to a `windows-rc` channel.
- [ ] Download through the itch app on a clean machine and retest launch, assets, saves, and uninstall behavior.
- [ ] Keep the page restricted or unlisted until clean-machine acceptance passes.

Official references:

- https://itch.io/docs/creators/getting-started
- https://itch.io/docs/butler/pushing.html

## Steam gate

- [ ] Complete Steamworks partner, tax, identity, and banking onboarding.
- [ ] Pay the Steam Direct app fee and record the earliest eligible release date.
- [ ] Create the app, depot, Windows launch option, and install directory.
- [ ] Prepare required capsule art, screenshots, descriptions, system requirements, and content survey.
- [ ] Publish Coming Soon for at least the required period.
- [ ] Upload the same accepted Windows package to a private SteamPipe branch.
- [ ] Install and run through the Steam client on a clean machine.
- [ ] Submit store presence and near-final build for Valve review with schedule margin.
- [ ] Do not advertise achievements, cloud saves, controller support, online multiplayer, or other features until each is verified in the uploaded build.

Official references:

- https://partner.steamgames.com/doc/gettingstarted/onboarding
- https://partner.steamgames.com/doc/store/Review_Process
- https://partner.steamgames.com/doc/sdk/uploading

## Later platforms

- macOS: sign with Developer ID, enable hardened runtime, notarize, staple, package, and test on both Intel and Apple Silicon before publishing.
- Linux: add a reproducible build and test on current Ubuntu plus Steam Deck before listing support.
- Google Play: create a release-signed Android App Bundle with required 64-bit ABI coverage, complete Play policy/privacy declarations, and run closed testing first.
- GOG/Epic/Microsoft Store: revisit after the first stable itch.io/Steam release has support documentation and real compatibility data.
