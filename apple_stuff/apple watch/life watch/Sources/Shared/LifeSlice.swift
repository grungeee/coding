import Foundation

struct LifeSlice: Hashable, Identifiable, Sendable {
    let period: LifePeriod
    let interval: DateInterval
    let now: Date

    var id: LifePeriod { period }

    var totalSeconds: TimeInterval {
        interval.duration
    }

    var elapsedSeconds: TimeInterval {
        min(totalSeconds, max(0, now.timeIntervalSince(interval.start)))
    }

    var remainingSeconds: TimeInterval {
        max(0, interval.end.timeIntervalSince(now))
    }

    var progressElapsed: Double {
        guard totalSeconds > 0 else { return 1 }
        return elapsedSeconds / totalSeconds
    }

    var progressRemaining: Double {
        guard totalSeconds > 0 else { return 0 }
        return remainingSeconds / totalSeconds
    }

    var percentRemaining: Double {
        progressRemaining * 100
    }

    var percentSpent: Double {
        progressElapsed * 100
    }

    var hoursRemaining: Double {
        remainingSeconds / 3600
    }

    var daysRemaining: Double {
        remainingSeconds / 86_400
    }

    var wholeHoursRemaining: Int {
        max(0, Int(hoursRemaining.rounded(.down)))
    }

    var wholeMinutesRemaining: Int {
        max(0, Int((remainingSeconds / 60).rounded(.down)))
    }

    var wholeDaysRemaining: Int {
        max(0, Int(daysRemaining.rounded(.down)))
    }

    var remainingMinutesPastHour: Int {
        max(0, wholeMinutesRemaining % 60)
    }

    var hoursRemainingText: String {
        Self.hoursFormatter.string(from: NSNumber(value: hoursRemaining)) ?? "0.0"
    }

    var daysRemainingText: String {
        Self.daysFormatter.string(from: NSNumber(value: daysRemaining)) ?? "0.0"
    }

    var percentRemainingText: String {
        Self.percentFormatter.string(from: NSNumber(value: percentRemaining)) ?? "0%"
    }

    var percentRemainingCompactText: String {
        Self.compactPercentFormatter.string(from: NSNumber(value: percentRemaining)) ?? "0%"
    }

    var percentSpentText: String {
        Self.percentFormatter.string(from: NSNumber(value: percentSpent)) ?? "0%"
    }

    var percentSpentCompactText: String {
        Self.compactPercentFormatter.string(from: NSNumber(value: percentSpent)) ?? "0%"
    }

    var dominantRemainingValue: String {
        switch period {
        case .day:
            return hoursRemainingText
        case .week, .month, .year:
            return daysRemainingText
        }
    }

    var dominantRemainingUnit: String {
        switch period {
        case .day:
            return "hours left"
        case .week, .month, .year:
            return "days left"
        }
    }

    var primaryRemainingText: String {
        switch period {
        case .day:
            let hours = wholeHoursRemaining
            let minutes = remainingMinutesPastHour
            let hourLabel = hours == 1 ? "hour" : "hours"
            let minuteLabel = minutes == 1 ? "minute" : "minutes"
            return "\(hours) \(hourLabel) and \(minutes) \(minuteLabel) left"
        case .week, .month, .year:
            return "\(dominantRemainingValue) \(dominantRemainingUnit)"
        }
    }

    var inlineRemainingCompact: String {
        switch period {
        case .day:
            return "\(hoursRemainingText)h"
        case .week, .month, .year:
            return "\(daysRemainingText)d"
        }
    }

    var widgetSummaryLine: String {
        switch period {
        case .day:
            return "\(hoursRemainingText) hours left"
        case .week, .month, .year:
            return "\(daysRemainingText) days left"
        }
    }

    var widgetDetailLine: String {
        "\(percentRemainingText) remaining"
    }

    var complicationValueText: String {
        switch period {
        case .day:
            return "\(wholeHoursRemaining)"
        case .week, .month, .year:
            return "\(wholeDaysRemaining)"
        }
    }

    private static let hoursFormatter = decimalFormatter(fractionDigits: 1)
    private static let daysFormatter = decimalFormatter(fractionDigits: 1)
    private static let percentFormatter = decimalFormatter(fractionDigits: 1, suffix: "%")
    private static let compactPercentFormatter = decimalFormatter(fractionDigits: 0, suffix: "%")

    private static func decimalFormatter(fractionDigits: Int, suffix: String = "") -> NumberFormatter {
        let formatter = NumberFormatter()
        formatter.numberStyle = .decimal
        formatter.maximumFractionDigits = fractionDigits
        formatter.minimumFractionDigits = fractionDigits
        formatter.locale = .autoupdatingCurrent
        formatter.usesGroupingSeparator = false
        formatter.positiveSuffix = suffix
        formatter.negativeSuffix = suffix
        return formatter
    }
}
