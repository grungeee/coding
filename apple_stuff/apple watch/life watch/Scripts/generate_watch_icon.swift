import AppKit

let outputURL = URL(fileURLWithPath: CommandLine.arguments.dropFirst().first ?? "Resources/App/Assets.xcassets/AppIcon.appiconset/LifeWatch-Base.png")
let size = CGSize(width: 1024, height: 1024)

let image = NSImage(size: size)
image.lockFocus()

let canvas = NSRect(origin: .zero, size: size)

let background = NSGradient(colors: [
    NSColor(calibratedRed: 0.09, green: 0.15, blue: 0.23, alpha: 1),
    NSColor(calibratedRed: 0.05, green: 0.08, blue: 0.12, alpha: 1),
])!
background.draw(in: canvas, angle: -35)

let outerRing = NSBezierPath(ovalIn: canvas.insetBy(dx: 120, dy: 120))
NSColor(calibratedRed: 0.25, green: 0.64, blue: 0.93, alpha: 0.18).setFill()
outerRing.fill()

let innerDisc = NSBezierPath(ovalIn: canvas.insetBy(dx: 184, dy: 184))
NSColor(calibratedRed: 0.05, green: 0.08, blue: 0.12, alpha: 1).setFill()
innerDisc.fill()

let arc = NSBezierPath()
arc.lineWidth = 48
arc.appendArc(withCenter: CGPoint(x: 512, y: 512), radius: 248, startAngle: 30, endAngle: 315, clockwise: false)
NSColor(calibratedRed: 0.95, green: 0.69, blue: 0.27, alpha: 1).setStroke()
arc.stroke()

let centerDot = NSBezierPath(ovalIn: NSRect(x: 472, y: 472, width: 80, height: 80))
NSColor.white.setFill()
centerDot.fill()

let hourHand = NSBezierPath()
hourHand.lineWidth = 42
hourHand.lineCapStyle = .round
hourHand.move(to: CGPoint(x: 512, y: 512))
hourHand.line(to: CGPoint(x: 512, y: 700))
NSColor.white.setStroke()
hourHand.stroke()

let minuteHand = NSBezierPath()
minuteHand.lineWidth = 30
minuteHand.lineCapStyle = .round
minuteHand.move(to: CGPoint(x: 512, y: 512))
minuteHand.line(to: CGPoint(x: 666, y: 420))
NSColor(calibratedRed: 0.25, green: 0.86, blue: 0.74, alpha: 1).setStroke()
minuteHand.stroke()

let paragraph = NSMutableParagraphStyle()
paragraph.alignment = .center

let attributes: [NSAttributedString.Key: Any] = [
    .font: NSFont.systemFont(ofSize: 108, weight: .black),
    .foregroundColor: NSColor.white.withAlphaComponent(0.9),
    .paragraphStyle: paragraph,
]

let text = NSString(string: "LW")
text.draw(in: NSRect(x: 0, y: 168, width: 1024, height: 130), withAttributes: attributes)

image.unlockFocus()

guard
    let tiff = image.tiffRepresentation,
    let bitmap = NSBitmapImageRep(data: tiff),
    let png = bitmap.representation(using: .png, properties: [:])
else {
    fatalError("Failed to encode icon PNG.")
}

try FileManager.default.createDirectory(
    at: outputURL.deletingLastPathComponent(),
    withIntermediateDirectories: true
)
try png.write(to: outputURL)
