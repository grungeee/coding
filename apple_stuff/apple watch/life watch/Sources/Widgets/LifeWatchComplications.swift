import SwiftUI
import WidgetKit

struct LifeEntry: TimelineEntry {
    let date: Date
    let period: LifePeriod
    let slice: LifeSlice
}

struct LifeTimelineProvider: TimelineProvider {
    let period: LifePeriod

    func placeholder(in context: Context) -> LifeEntry {
        previewEntry(at: .now)
    }

    func getSnapshot(in context: Context, completion: @escaping @Sendable (LifeEntry) -> Void) {
        completion(previewEntry(at: .now))
    }

    func getTimeline(in context: Context, completion: @escaping @Sendable (Timeline<LifeEntry>) -> Void) {
        let start = Date()
        let entries = (0..<96).map { step -> LifeEntry in
            let date = start.addingTimeInterval(Double(step) * 900)
            return previewEntry(at: date)
        }

        completion(Timeline(
            entries: entries,
            policy: .after(LifeWatchCalculator.nextRefreshDate(after: start))
        ))
    }

    private func previewEntry(at date: Date) -> LifeEntry {
        let slice = LifeWatchCalculator.slice(for: period, now: date)
        return LifeEntry(date: date, period: period, slice: slice)
    }
}

struct LifeWatchDaySpentWidget: Widget {
    private let kind = "LifeWatchDaySpentComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .day)) { entry in
            LifeComplicationView(entry: entry, mode: .percentSpent)
        }
        .configurationDisplayName("Life Watch Day Percent")
        .description("Circular complication showing the percent of the day already spent.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchDayLeftWidget: Widget {
    private let kind = "LifeWatchDayLeftComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .day)) { entry in
            LifeComplicationView(entry: entry, mode: .valueLeft)
        }
        .configurationDisplayName("Life Watch Day Left")
        .description("Circular complication showing the whole hours left in the day.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchWeekSpentWidget: Widget {
    private let kind = "LifeWatchWeekSpentComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .week)) { entry in
            LifeComplicationView(entry: entry, mode: .percentSpent)
        }
        .configurationDisplayName("Life Watch Week Percent")
        .description("Circular complication showing the percent of the week already spent.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchWeekLeftWidget: Widget {
    private let kind = "LifeWatchWeekLeftComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .week)) { entry in
            LifeComplicationView(entry: entry, mode: .valueLeft)
        }
        .configurationDisplayName("Life Watch Week Left")
        .description("Circular complication showing the whole days left in the week.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchMonthSpentWidget: Widget {
    private let kind = "LifeWatchMonthSpentComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .month)) { entry in
            LifeComplicationView(entry: entry, mode: .percentSpent)
        }
        .configurationDisplayName("Life Watch Month Percent")
        .description("Circular complication showing the percent of the month already spent.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchMonthLeftWidget: Widget {
    private let kind = "LifeWatchMonthLeftComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .month)) { entry in
            LifeComplicationView(entry: entry, mode: .valueLeft)
        }
        .configurationDisplayName("Life Watch Month Left")
        .description("Circular complication showing the whole days left in the month.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchYearSpentWidget: Widget {
    private let kind = "LifeWatchYearSpentComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .year)) { entry in
            LifeComplicationView(entry: entry, mode: .percentSpent)
        }
        .configurationDisplayName("Life Watch Year Percent")
        .description("Circular complication showing the percent of the year already spent.")
        .supportedFamilies([.accessoryCircular])
    }
}

struct LifeWatchYearLeftWidget: Widget {
    private let kind = "LifeWatchYearLeftComplication"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: LifeTimelineProvider(period: .year)) { entry in
            LifeComplicationView(entry: entry, mode: .valueLeft)
        }
        .configurationDisplayName("Life Watch Year Left")
        .description("Circular complication showing the whole days left in the year.")
        .supportedFamilies([.accessoryCircular])
    }
}

@main
struct LifeWatchComplicationsBundle: WidgetBundle {
    var body: some Widget {
        LifeWatchDaySpentWidget()
        LifeWatchDayLeftWidget()
        LifeWatchWeekSpentWidget()
        LifeWatchWeekLeftWidget()
        LifeWatchMonthSpentWidget()
        LifeWatchMonthLeftWidget()
        LifeWatchYearSpentWidget()
        LifeWatchYearLeftWidget()
    }
}

private struct LifeComplicationView: View {
    let entry: LifeEntry
    let mode: LifeCircularComplicationMode

    var body: some View {
        LifeCircularGaugeView(slice: entry.slice, mode: mode)
    }
}

#Preview(as: .accessoryCircular) {
    LifeWatchYearSpentWidget()
} timeline: {
    LifeEntry(
        date: .now,
        period: .year,
        slice: LifeWatchCalculator.slice(for: .year)
    )
}
