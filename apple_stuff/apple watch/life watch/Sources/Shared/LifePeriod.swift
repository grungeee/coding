import Foundation
import SwiftUI

enum LifePeriod: String, CaseIterable, Codable, Hashable, Identifiable, Sendable {
    case day
    case week
    case month
    case year

    var id: String { rawValue }

    var title: String {
        switch self {
        case .day:
            return "Day"
        case .week:
            return "Week"
        case .month:
            return "Month"
        case .year:
            return "Year"
        }
    }

    var shortTitle: String {
        switch self {
        case .day:
            return "Day"
        case .week:
            return "Week"
        case .month:
            return "Month"
        case .year:
            return "Year"
        }
    }

    var symbolName: String {
        switch self {
        case .day:
            return "sun.max.fill"
        case .week:
            return "calendar.badge.clock"
        case .month:
            return "calendar"
        case .year:
            return "sparkles"
        }
    }

    var tint: Color {
        switch self {
        case .day:
            return Color(red: 0.92, green: 0.60, blue: 0.16)
        case .week:
            return Color(red: 0.11, green: 0.73, blue: 0.54)
        case .month:
            return Color(red: 0.12, green: 0.55, blue: 0.98)
        case .year:
            return Color(red: 0.73, green: 0.33, blue: 0.93)
        }
    }
}

enum LifeCircularComplicationMode: Hashable, Sendable {
    case percentSpent
    case valueLeft

    var suffixTitle: String {
        switch self {
        case .percentSpent:
            return "Percent"
        case .valueLeft:
            return "Left"
        }
    }

    func centerText(for slice: LifeSlice) -> String {
        switch self {
        case .percentSpent:
            return slice.percentSpentCompactText
        case .valueLeft:
            return slice.complicationValueText
        }
    }
}

struct LifeCircularGaugeView: View {
    let slice: LifeSlice
    let mode: LifeCircularComplicationMode

    var body: some View {
        Gauge(value: slice.progressElapsed) {
            Image(systemName: slice.period.symbolName)
        } currentValueLabel: {
            Text(mode.centerText(for: slice))
        }
        .gaugeStyle(.accessoryCircularCapacity)
        .tint(slice.progressTint)
    }
}

extension LifeSlice {
    var progressTint: Color {
        let clamped = min(max(progressElapsed, 0), 1)
        let hue = (1 - clamped) * 0.33
        return Color(hue: hue, saturation: 0.92, brightness: 0.96)
    }
}
