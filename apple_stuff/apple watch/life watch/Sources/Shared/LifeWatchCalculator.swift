import Foundation

enum LifeWatchCalculator {
    static func allSlices(now: Date = .now, calendar: Calendar = .autoupdatingCurrent) -> [LifeSlice] {
        LifePeriod.allCases.map { slice(for: $0, now: now, calendar: calendar) }
    }

    static func slice(
        for period: LifePeriod,
        now: Date = .now,
        calendar: Calendar = .autoupdatingCurrent
    ) -> LifeSlice {
        let interval = interval(for: period, now: now, calendar: calendar)
        return LifeSlice(period: period, interval: interval, now: now)
    }

    static func nextRefreshDate(
        after date: Date = .now,
        calendar: Calendar = .autoupdatingCurrent,
        refreshMinutes: Int = 15
    ) -> Date {
        let minute = calendar.component(.minute, from: date)
        let remainder = minute % refreshMinutes
        let minutesToAdd = remainder == 0 ? refreshMinutes : refreshMinutes - remainder
        let nextDate = calendar.date(byAdding: .minute, value: minutesToAdd, to: date) ?? date.addingTimeInterval(900)
        return calendar.date(bySetting: .second, value: 0, of: nextDate) ?? nextDate
    }

    private static func interval(
        for period: LifePeriod,
        now: Date,
        calendar: Calendar
    ) -> DateInterval {
        let component: Calendar.Component
        switch period {
        case .day:
            component = .day
        case .week:
            component = .weekOfYear
        case .month:
            component = .month
        case .year:
            component = .year
        }

        guard let interval = calendar.dateInterval(of: component, for: now) else {
            fatalError("Unable to determine the current \(period.title.lowercased()) interval.")
        }

        return interval
    }
}
