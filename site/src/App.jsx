import Nav from "./components/Nav.jsx";
import Hero from "./components/Hero.jsx";
import Tour from "./components/Tour.jsx";
import Tools from "./components/Tools.jsx";
import Install from "./components/Install.jsx";
import Keys from "./components/Keys.jsx";
import Footer from "./components/Footer.jsx";
import { VERSION } from "./data.js";

export default function App() {
  return (
    <>
      <a
        href="#tour"
        className="sr-only focus:not-sr-only focus:absolute focus:left-4 focus:top-4 focus:z-50 focus:border-3 focus:border-ink focus:bg-yellow focus:px-4 focus:py-2 focus:font-bold"
      >
        Skip to content
      </a>
      <Nav version={VERSION} />
      <main>
        <Hero version={VERSION} />
        <Tour />
        <Tools />
        <Install />
        <Keys />
      </main>
      <Footer />
    </>
  );
}
