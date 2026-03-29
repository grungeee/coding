import SwiftUI

struct ContentView: View {
    var body: some View {
        TimelineView(.periodic(from: .now, by: 60)) { context in
            let slices = LifeWatchCalculator.allSlices(now: context.date)

            ZStack {
                LinearGradient(
                    colors: [
                        Color(red: 0.05, green: 0.08, blue: 0.12),
                        Color(red: 0.08, green: 0.15, blue: 0.23),
                    ],
                    startPoint: .topLeading,
                    endPoint: .bottomTrailing
                )
                .ignoresSafeArea()

                ScrollView {
                    VStack(alignment: .leading, spacing: 12) {
                        header(now: context.date)

                        ForEach(slices) { slice in
                            MetricCardView(slice: slice)
                        }

                        ComplicationLabView(slices: slices)

                        Text("Add a Life Watch circular complication from the watch face. Each period has a % spent version and a left-value version.")
                            .font(.footnote)
                            .foregroundStyle(.white.opacity(0.72))
                            .padding(.top, 4)
                    }
                    .padding(12)
                }
            }
        }
    }

    @ViewBuilder
    private func header(now: Date) -> some View {
        VStack(alignment: .leading, spacing: 4) {
            Text("Life Watch")
                .font(.system(.title3, design: .rounded, weight: .bold))
                .foregroundStyle(.white)

            Text(now.formatted(date: .abbreviated, time: .shortened))
                .font(.footnote)
                .foregroundStyle(.white.opacity(0.72))
        }
    }
}

private struct ComplicationLabView: View {
    let slices: [LifeSlice]

    private let columns = [
        GridItem(.flexible(), spacing: 10),
        GridItem(.flexible(), spacing: 10),
    ]

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            Text("Complication Lab")
                .font(.system(.headline, design: .rounded, weight: .bold))
                .foregroundStyle(.white)

            LazyVGrid(columns: columns, spacing: 10) {
                ForEach(slices) { slice in
                    ComplicationPreviewTile(slice: slice, mode: .percentSpent)
                    ComplicationPreviewTile(slice: slice, mode: .valueLeft)
                }
            }
        }
    }
}

private struct MetricCardView: View {
    let slice: LifeSlice

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            HStack(alignment: .top) {
                VStack(alignment: .leading, spacing: 3) {
                    Label(slice.period.title, systemImage: slice.period.symbolName)
                        .font(.system(.headline, design: .rounded, weight: .semibold))
                        .foregroundStyle(slice.period.tint)

                    Text(slice.primaryRemainingText)
                        .font(.system(.title3, design: .rounded, weight: .bold))
                        .foregroundStyle(.white)
                        .minimumScaleFactor(0.8)
                }

                Spacer(minLength: 8)

                VStack(alignment: .trailing, spacing: 2) {
                    Text("SPENT")
                        .font(.system(.caption2, design: .rounded, weight: .bold))
                        .foregroundStyle(.white.opacity(0.52))

                    Text(slice.percentSpentText)
                        .font(.system(.title3, design: .rounded, weight: .heavy))
                        .foregroundStyle(.white)
                }
            }

            HStack(spacing: 10) {
                stat(label: "Hours", value: slice.hoursRemainingText)
                stat(label: "Days", value: slice.daysRemainingText)
            }

            ProgressView(value: slice.progressElapsed)
                .tint(slice.progressTint)
        }
        .padding(12)
        .background(
            RoundedRectangle(cornerRadius: 18, style: .continuous)
                .fill(Color.white.opacity(0.08))
                .overlay(
                    RoundedRectangle(cornerRadius: 18, style: .continuous)
                        .stroke(Color.white.opacity(0.08), lineWidth: 1)
                )
        )
    }

    @ViewBuilder
    private func stat(label: String, value: String) -> some View {
        VStack(alignment: .leading, spacing: 2) {
            Text(label.uppercased())
                .font(.system(.caption2, design: .rounded, weight: .bold))
                .foregroundStyle(.white.opacity(0.52))

            Text(value)
                .font(.system(.body, design: .rounded, weight: .semibold))
                .foregroundStyle(.white.opacity(0.92))
        }
        .frame(maxWidth: .infinity, alignment: .leading)
    }
}

private struct ComplicationPreviewTile: View {
    let slice: LifeSlice
    let mode: LifeCircularComplicationMode

    var body: some View {
        VStack(spacing: 8) {
            LifeCircularGaugeView(slice: slice, mode: mode)
                .frame(width: 64, height: 64)

            Text("\(slice.period.shortTitle) \(mode == .percentSpent ? "%" : "Left")")
                .font(.system(.caption2, design: .rounded, weight: .bold))
                .foregroundStyle(.white.opacity(0.82))
                .multilineTextAlignment(.center)
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 10)
        .background(
            RoundedRectangle(cornerRadius: 16, style: .continuous)
                .fill(Color.white.opacity(0.06))
                .overlay(
                    RoundedRectangle(cornerRadius: 16, style: .continuous)
                        .stroke(Color.white.opacity(0.08), lineWidth: 1)
                )
        )
    }
}

#Preview {
    ContentView()
}
