document.addEventListener('alpine:init', () => {
  Alpine.data('pressure', () => ({
    chart: null,
    running: false,
    timer: null,

    // live-series state
    labels: [],          // simple 1..N for x-axis
    points: [],          // y values
    counter: 0,          // label counter
    lastValue: 12,       // start value (kPa)
    maxPoints: 120,      // sliding window length
    intervalMs: 200,     // update period

    initChart() {
      const ctx = this.$refs.chart.getContext('2d');
      this.chart = new Chart(ctx, {
        type: 'line',
        data: {
          labels: this.labels,
          datasets: [{
            label: 'Pressure (kPa)',
            data: this.points,
            borderWidth: 2,
            borderColor: 'rgba(0,0,0,0.8)',
            backgroundColor: 'rgba(0,0,0,0.1)',
            tension: 0.25,
            pointRadius: 0,         // smoother, faster
          }]
        },
        options: {
          responsive: true,
          maintainAspectRatio: false,
          animation: false,         // key for realtime perf
          parsing: false,
          scales: {
            x: { ticks: { maxTicksLimit: 8 } },
            y: { beginAtZero: true }
          },
          plugins: {
            legend: { display: true }
          }
        }
      });
    },

    // Start/stop/pause
    toggle() {
      this.running ? this.stop() : this.start();
    },
    start() {
      if (this.running) return;
      this.running = true;
      this.timer = setInterval(() => {
        // Replace this with your real sensor value
        const next = this.simulateNextKpa();

        this.pushPoint(next);
      }, this.intervalMs);
    },
    stop() {
      this.running = false;
      clearInterval(this.timer);
      this.timer = null;
    },
    restartIfRunning() {
      if (this.running) { this.stop(); this.start(); }
    },

    // Data handling
    pushPoint(y) {
      this.lastValue = y;
      this.counter += 1;
      this.labels.push(this.counter);
      this.points.push(y);

      if (this.points.length > this.maxPoints) {
        this.labels.shift();
        this.points.shift();
      }
      this.chart.update('none'); // no animation for realtime
    },
    clearData() {
      this.labels.length = 0;
      this.points.length = 0;
      this.counter = 0;
      this.chart.update('none');
    },

    // Simple random-walk simulator to mimic sensor drift
    simulateNextKpa() {
      const jitter = (Math.random() - 0.5) * 0.6; // +/-0.3
      let v = this.lastValue + jitter;
      if (v < 0) v = 0;
      return v;
    },
  }));
});
