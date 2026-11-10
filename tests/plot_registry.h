/* The plot registry is the source of truth for scripts, inputs, PNGs, and counts. */
#ifndef TICKTRADE_PLOT_REGISTRY_H
#define TICKTRADE_PLOT_REGISTRY_H

typedef struct {
    const char *name;
    const char *script;
    const char *input1;
    const char *input2;
    const char *headers;
    int minimum_rows;
    const char *png;
    unsigned width;
    unsigned height;
    const char *summary;
    const char *log;
    long minimum_png_bytes;
    const char *series;
} PlotSpec;

static const PlotSpec plot_registry[] = {
    {"signals", "scripts/plot_signals.gp", "results/plots/signals.dat", "results/plots/signal_points.dat", "", 1,
     "results/plots/signals.png", 1400, 700, "results/logs/plot_signals_summary.txt", "results/logs/gnuplot_signals.log", 10000,
     "trade_price,short_sma,long_sma,buy,sell"},
    {"money-lost", "scripts/plot_money_lost.gp", "results/rival/sweep.csv", "", "platform,member,reaction_us,pnl,money_lost,trades", 25,
     "results/plots/money_lost.png", 1400, 700, "results/logs/plot_money_lost_summary.txt", "results/logs/gnuplot_money_lost.log", 5000,
     "money_lost"},
    {"ladder", "scripts/plot_ladder.gp", "results/bench/ladder_plot_input.csv", "", "platform,member,version,file,trades,repeats,median_total_ns,best_total_ns,worst_total_ns,median_process_ns,trades_per_sec,peak_mem_kb,compiler,flags", 1,
     "results/plots/ladder.png", 1400, 700, "results/logs/plot_ladder_summary.txt", "results/logs/gnuplot_ladder.log", 5000,
     "median_total_ns"}
};
#define PLOT_REGISTRY_COUNT (sizeof(plot_registry) / sizeof(plot_registry[0]))
#endif
