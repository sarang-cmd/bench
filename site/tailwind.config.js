/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,jsx}"],
  theme: {
    extend: {
      colors: {
        paper: "#f4f3ee",
        ink: "#1a1818",
        red: "#d92d27",
        yellow: "#f2c230",
        blue: "#2f5fd0",
        teal: "#1f9c8a",
        violet: "#8b5cf6",
        shade: "#d9d7cd",
      },
      fontFamily: {
        sans: ["Inter", "DejaVu Sans", "Helvetica Neue", "Arial", "sans-serif"],
        mono: ["DejaVu Sans Mono", "ui-monospace", "SFMono-Regular", "monospace"],
      },
      boxShadow: {
        brute: "8px 8px 0 #1a1818",
        "brute-sm": "5px 5px 0 #1a1818",
        "brute-lg": "12px 12px 0 #1a1818",
      },
      borderWidth: { 3: "3px" },
    },
  },
  plugins: [],
};
