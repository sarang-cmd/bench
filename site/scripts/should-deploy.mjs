// should-deploy.mjs - the Ignored Build Step for the site.
//
// Vercel runs this before building and skips the deployment when it exits 0.
// A site change lands on `main` every few commits, and rebuilding and
// re-deploying for each one of those is noise: the preview of the pull request
// already proved the change renders, and the production URL only needs to be a
// little behind the branch.
//
// Rules, in order:
//
//   1. `[deploy]` anywhere in the commit message   -> deploy, always.
//      That is the manual switch: put it in a commit, or run a deployment from
//      the Vercel dashboard, and the site is live.
//   2. fewer than MIN_COMMITS commits since the last deployment -> skip.
//   3. otherwise                                    -> deploy.
//
// Vercel hands the build the SHA of the previous deployment in
// VERCEL_GIT_PREVIOUS_SHA. When it is missing or the clone is too shallow to
// count that far back, the build goes ahead: a site that is one deployment
// early is a nuisance, a site that never updates is a bug.
import { execFileSync } from "node:child_process";

const MIN_COMMITS = 20;

function git(args) {
  return execFileSync("git", args, { encoding: "utf8", stdio: ["ignore", "pipe", "ignore"] }).trim();
}

function decide() {
  const message = process.env.VERCEL_GIT_COMMIT_MESSAGE ?? "";
  if (/\[deploy\]/i.test(message)) return ["deploy", "the commit asks for a deployment"];

  const previous = process.env.VERCEL_GIT_PREVIOUS_SHA;
  if (!previous) return ["deploy", "no previous deployment to compare against"];

  // Vercel clones with a short history; deepen it before counting.
  for (const depth of ["", "50", "200"]) {
    try {
      if (depth) git(["fetch", "--quiet", "--deepen", depth]);
      const count = Number(git(["rev-list", "--count", `${previous}..HEAD`]));
      if (Number.isFinite(count)) {
        return count >= MIN_COMMITS
          ? ["deploy", `${count} commits since ${previous.slice(0, 7)}`]
          : ["skip", `only ${count} of ${MIN_COMMITS} commits since ${previous.slice(0, 7)}`];
      }
    } catch {
      // try a deeper fetch, then give up and deploy
    }
  }
  return ["deploy", "the history is too shallow to count, so this build goes ahead"];
}

const [decision, reason] = decide();
console.log(`should-deploy: ${decision} - ${reason}`);
process.exit(decision === "skip" ? 0 : 1);
