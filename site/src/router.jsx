// A very small router.  The site is one Vite bundle on Vercel, so the seven
// pages are a path switch here plus one rewrite rule in vercel.json; no router
// dependency, no build step that has to know about routes.
//
// Every route owns a title and a description, applied whenever the path
// changes, so a shared link and a search result say the right thing.
import { useEffect, useState } from "react";

export const SITE = "https://bench-gdc.vercel.app";

export const ROUTES = [
  {
    path: "/",
    label: "Home",
    title: "Bench - 18 tools in one Casio fx-CG 50 add-in",
    description:
      "One .g3a for the Casio fx-CG 50: a paper pacer, uncertainty arithmetic, flashcards, a Markdown reader, study log, solvers and statistics.",
  },
  {
    path: "/features",
    label: "Features",
    title: "Features - Bench for the Casio fx-CG 50",
    description:
      "Every tool in Bench, one card each, with real screenshots taken from the add-in running on a PC frame buffer.",
  },
  {
    path: "/simulator",
    label: "Simulator",
    title: "Simulator - Bench for the Casio fx-CG 50",
    description:
      "Run the add-in itself in the browser: the same C compiled to WebAssembly, driven by the key buttons, plus every captured screen.",
  },
  {
    path: "/tutorial",
    label: "Tutorial",
    title: "Tutorial - Bench for the Casio fx-CG 50",
    description:
      "Ten minutes from first launch to a study session: the home screen, adding a note, a card session, the study log and Settings.",
  },
  {
    path: "/install",
    label: "Install",
    title: "Install - Bench for the Casio fx-CG 50",
    description:
      "Copy one .g3a to the calculator in USB Flash mode, check the SHA-256, and know the three things that block a launch.",
  },
  {
    path: "/downloads",
    label: "Downloads",
    title: "Downloads - Bench for the Casio fx-CG 50",
    description:
      "The same .g3a from four places: this site, the GitHub release, the dist/ folder in the repository and the builds branch.",
  },
  {
    path: "/changelog",
    label: "Changelog",
    title: "Changelog - Bench for the Casio fx-CG 50",
    description:
      "What changed in every version of Bench, generated from CHANGELOG.md so the page cannot drift from the file.",
  },
  {
    path: "/about",
    label: "About",
    title: "About - Bench for the Casio fx-CG 50",
    description:
      "What Bench is, what it is not, how it is built and tested, and what has been checked on real hardware.",
  },
];

function stripTrailingSlash(path) {
  if (path.length > 1 && path.endsWith("/")) return path.slice(0, -1);
  return path;
}

export function currentPath() {
  if (typeof window === "undefined") return "/";
  return stripTrailingSlash(window.location.pathname || "/");
}

export function navigate(to) {
  if (typeof window === "undefined") return;
  const path = stripTrailingSlash(to);
  if (path === currentPath()) return;
  window.history.pushState({}, "", path);
  window.dispatchEvent(new PopStateEvent("popstate"));
  window.scrollTo(0, 0);
}

// Reads the path out of the address bar and re-renders on back/forward.
export function usePath() {
  const [path, setPath] = useState(currentPath);

  useEffect(() => {
    const onChange = () => setPath(currentPath());
    window.addEventListener("popstate", onChange);
    return () => window.removeEventListener("popstate", onChange);
  }, []);

  return path;
}

export function routeFor(path) {
  return ROUTES.find((route) => route.path === path) || ROUTES[0];
}

// Title, description, canonical and the Open Graph tags, per route.
export function usePageMeta(route) {
  useEffect(() => {
    const image = `${SITE}/images/og${route.path === "/" ? "" : route.path.replace(/\//g, "-")}.png`;
    const url = `${SITE}${route.path}`;

    document.title = route.title;
    setMeta("name", "description", route.description);
    setMeta("property", "og:title", route.title);
    setMeta("property", "og:description", route.description);
    setMeta("property", "og:url", url);
    setMeta("property", "og:image", image);
    setMeta("name", "twitter:title", route.title);
    setMeta("name", "twitter:description", route.description);
    setMeta("name", "twitter:image", image);
    setLink("canonical", url);
  }, [route]);
}

function setMeta(attr, key, value) {
  let tag = document.head.querySelector(`meta[${attr}="${key}"]`);
  if (!tag) {
    tag = document.createElement("meta");
    tag.setAttribute(attr, key);
    document.head.appendChild(tag);
  }
  tag.setAttribute("content", value);
}

function setLink(rel, href) {
  let tag = document.head.querySelector(`link[rel="${rel}"]`);
  if (!tag) {
    tag = document.createElement("link");
    tag.setAttribute("rel", rel);
    document.head.appendChild(tag);
  }
  tag.setAttribute("href", href);
}

// A plain <a> that stays inside the app.  The href is real, so middle-click,
// ctrl-click and a right-click "copy link" all do the ordinary thing.
export function Link({ to, children, className = "", onClick, ...rest }) {
  return (
    <a
      href={to}
      className={className}
      onClick={(event) => {
        if (onClick) onClick(event);
        if (event.defaultPrevented) return;
        if (event.metaKey || event.ctrlKey || event.shiftKey || event.button !== 0) return;
        event.preventDefault();
        navigate(to);
      }}
      {...rest}
    >
      {children}
    </a>
  );
}
