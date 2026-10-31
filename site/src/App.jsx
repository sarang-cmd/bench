import { Analytics } from "@vercel/analytics/react";
import Nav from "./components/Nav.jsx";
import Hero from "./components/Hero.jsx";
import Tour from "./components/Tour.jsx";
import Tools from "./components/Tools.jsx";
import Install from "./components/Install.jsx";
import Keys from "./components/Keys.jsx";
import Footer from "./components/Footer.jsx";
import { VERSION } from "./data.js";
import { ROUTES, routeFor, usePageMeta, usePath } from "./router.jsx";
import {
  AboutPage,
  ChangelogPage,
  DownloadsPage,
  FeaturesPage,
  InstallPage,
  SimulatorPage,
  TutorialPage,
} from "./pages.jsx";

// The landing page is the V1 page, unchanged: hero, screen tour, tools, install,
// keys.  Every other route is one of the pages in pages.jsx.
function Home() {
  return (
    <>
      <Hero version={VERSION} />
      <Tour />
      <Tools />
      <Install />
      <Keys />
    </>
  );
}

const VIEWS = {
  "/": Home,
  "/features": FeaturesPage,
  "/simulator": SimulatorPage,
  "/tutorial": TutorialPage,
  "/install": InstallPage,
  "/downloads": DownloadsPage,
  "/changelog": ChangelogPage,
  "/about": AboutPage,
};

function NotFound() {
  return (
    <div className="px-5 py-20 sm:px-8 lg:px-12">
      <div className="mx-auto w-full max-w-6xl">
        <p className="font-mono text-xs uppercase tracking-widest text-ink/60">404</p>
        <h1 className="mt-2 text-3xl font-black sm:text-4xl">No such screen.</h1>
        <p className="mt-3 max-w-xl text-base text-ink/75">
          That page is not part of Bench. The menu is the list of everything that is:
        </p>
        <ul className="mt-6 flex flex-wrap gap-3">
          {ROUTES.map((route) => (
            <li key={route.path}>
              <a
                href={route.path}
                className="inline-block border-3 border-ink bg-paper px-3 py-1.5 font-mono text-xs uppercase tracking-widest hover:bg-yellow"
              >
                {route.label}
              </a>
            </li>
          ))}
        </ul>
      </div>
    </div>
  );
}

export default function App() {
  const path = usePath();
  const route = VIEWS[path] ? routeFor(path) : null;
  const View = VIEWS[path] || NotFound;

  usePageMeta(
    route || {
      path: path.startsWith("/") ? path : "/",
      title: "No such screen - Bench for the Casio fx-CG 50",
      description: "That page is not part of Bench.",
    }
  );

  return (
    <>
      <a
        href="#top"
        className="sr-only focus:not-sr-only focus:absolute focus:left-4 focus:top-4 focus:z-50 focus:border-3 focus:border-ink focus:bg-yellow focus:px-4 focus:py-2 focus:font-bold"
      >
        Skip to content
      </a>
      <Nav version={VERSION} />
      <main id="top">
        <View />
      </main>
      <Footer />
      <Analytics />
    </>
  );
}
