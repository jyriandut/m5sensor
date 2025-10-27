document.addEventListener("alpine:init", () => {
    Alpine.data("pressure", () => ({
        connected: false,
        socket: null,
        chart: null,
        dataPoints: [],
        labels: [],

        init() {
            this.initChart();
            this.connectWebSocket();
        },

        connectWebSocket() {
            const wsUrl = "ws://192.168.39.133/ws";
            this.socket = new WebSocket(wsUrl);

            this.socket.addEventListener("open", () => {
                console.log("Connected to pressure source");
                this.connected = true;
            });

            this.socket.addEventListener("close", () => {
                console.log("Disconnected from pressure source");
                this.connected = false;
                setTimeout(() => this.connectWebSocket(), 2000);
            });

            this.socket.addEventListener("message", (event) => {
                const pressure = parseFloat(event.data);
                if (!isNaN(pressure)) {
                    this.addDataPoint(pressure);
                } else {
                    console.warn("Invalid pressure data:", event.data);
                }
            });

            this.socket.addEventListener("error", (err) => {
                console.error("WebSocket Error", err);
                this.connected = false;
            });
        },

        initChart() {
            const ctx = this.$refs.chart.getContext("2d");

            // 🔧 destroy existing chart if it exists
            if (this.chart) {
                this.chart.destroy();
                this.chart = null;
            }

            this.chart = new Chart(ctx, {
                type: "line",
                data: {
                    labels: [],
                    datasets: [
                        {
                            label: "Pressure (kPa)",
                            data: [],
                            borderWidth: 2,
                            borderColor: "rgba(0, 128, 255, 0.8)",
                            backgroundColor: "rgba(0, 128, 255, 0.1)",
                            tension: 0.25,
                            pointRadius: 0,
                        },
                    ],
                },
                options: {
                    animation: true,
                    responsive: true,
                    maintainAspectRatio: false,
                    scales: {
                        x: { ticks: { maxTicksLimit: 8 } },
                        y: { beginAtZero: true },
                    },
                    plugins: { legend: { display: true } },
                },
            });
        },

        addDataPoint(value) {
            const maxPoints = 50;
            const label = new Date().toLocaleTimeString("en-US", {
                minute: "2-digit",
                second: "2-digit",
            });

            this.labels.push(label);
            this.dataPoints.push(value);

            if (this.labels.length > maxPoints) {
                this.labels.shift();
                this.dataPoints.shift();
            }

            this.chart.data.labels = this.labels;
            this.chart.data.datasets[0].data = this.dataPoints;
            this.chart.update(); // update instantly, no animation
        },
    }));
});
