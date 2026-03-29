# Life Watch

`Life Watch` is a standalone watchOS app plus a WidgetKit complication extension for Apple Watch.

It shows how much of the current:

- day
- week
- month
- year

is left, expressed as:

- hours
- days
- percentage remaining

## What is included

- A SwiftUI watch app that updates every minute.
- A complication/widget extension that supports inline, circular, corner, and rectangular families.
- Four complication variants so each placement can track `Day`, `Week`, `Month`, or `Year`.

## Local setup

1. Install full Xcode from Apple.
2. Point the active developer directory at Xcode, for example:

```bash
sudo xcode-select -s /Applications/Xcode.app
```

3. Generate the project:

```bash
xcodegen generate
```

4. Open `Life Watch.xcodeproj` in Xcode.
5. Pick an Apple Watch Ultra 2 simulator or device and run the `Life Watch` scheme.

## Testing on Apple Watch Ultra 2

1. In Xcode, open the `Life Watch` scheme.
2. Choose an `Apple Watch Ultra 2 (49mm)` simulator, or a paired physical Apple Watch.
3. Build and run the watch app once so it is installed.
4. Long-press the watch face, tap `Edit`, then add a complication slot.
5. Choose `Life Watch`.
6. Pick the specific `Life Watch` variant you want: `Day`, `Week`, `Month`, or `Year`.

## Regenerating the placeholder icon

If you want to refresh the temporary icon artwork:

```bash
swift Scripts/generate_watch_icon.swift
```

## Notes

- This machine only had Command Line Tools installed, so the project could be generated but not compiled here.
- The current icon set is a generated placeholder that is good enough for local testing but should be replaced before shipping.
