import { render, screen, fireEvent, waitFor, within } from "@testing-library/react";
import { ContainersPage } from "../pages/ContainersPage";

// ── fetch mock helpers ────────────────────────────────────────────────────────

function mockFetch(responses: Record<string, unknown>): void {
  vi.spyOn(global, "fetch").mockImplementation(async (input) => {
    const url = input.toString();
    for (const [pattern, body] of Object.entries(responses)) {
      if (url.includes(pattern)) {
        return new Response(JSON.stringify(body), {
          status: 200,
          headers: { "Content-Type": "application/json" },
        });
      }
    }
    return new Response(JSON.stringify([]), { status: 200 });
  });
}

const RUNNING: unknown[] = [
  { name: "web-01", type: "container", status: "Running", image: "ubuntu/24.04", project: "default", remote: "local" },
];
const STOPPED: unknown[] = [
  { name: "db-01", type: "container", status: "Stopped", image: "postgres:16", project: "default", remote: "local" },
];
const MIXED: unknown[] = [...RUNNING, ...STOPPED];

// SSE: useEvents opens an EventSource — stub it so it never fires
beforeAll(() => {
  (global as unknown as Record<string, unknown>).EventSource = class {
    onmessage: ((e: MessageEvent) => void) | null = null;
    onerror:   ((e: Event) => void) | null = null;
    addEventListener() {}
    removeEventListener() {}
    close() {}
  };
});

afterEach(() => {
  vi.restoreAllMocks();
});

// ── rendering ─────────────────────────────────────────────────────────────────

it("shows loading state initially", () => {
  mockFetch({ "/api/v1/instances": MIXED });
  render(<ContainersPage />);
  expect(screen.getByText(/loading/i)).toBeInTheDocument();
});

it("renders container rows after fetch", async () => {
  mockFetch({ "/api/v1/instances": MIXED });
  render(<ContainersPage />);
  await waitFor(() => expect(screen.getByText("web-01")).toBeInTheDocument());
  expect(screen.getByText("db-01")).toBeInTheDocument();
});

it("shows empty state when no containers returned", async () => {
  mockFetch({ "/api/v1/instances": [] });
  render(<ContainersPage />);
  await waitFor(() => expect(screen.getByText(/no containers found/i)).toBeInTheDocument());
});

it("shows error message on fetch failure", async () => {
  vi.spyOn(global, "fetch").mockRejectedValue(new Error("Network error"));
  render(<ContainersPage />);
  await waitFor(() => expect(screen.getByText(/network error/i)).toBeInTheDocument());
});

it("renders StatusBadge for each row", async () => {
  mockFetch({ "/api/v1/instances": MIXED });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  expect(screen.getByText("Running")).toBeInTheDocument();
  expect(screen.getByText("Stopped")).toBeInTheDocument();
});

// ── action buttons ────────────────────────────────────────────────────────────

it("shows Stop/Restart/Freeze for Running containers", async () => {
  mockFetch({ "/api/v1/instances": RUNNING });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  expect(screen.getByRole("button", { name: "Stop" })).toBeInTheDocument();
  expect(screen.getByRole("button", { name: "Restart" })).toBeInTheDocument();
  expect(screen.getByRole("button", { name: "Freeze" })).toBeInTheDocument();
});

it("shows Start for Stopped containers", async () => {
  mockFetch({ "/api/v1/instances": STOPPED });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("db-01"));
  expect(screen.getByRole("button", { name: "Start" })).toBeInTheDocument();
  expect(screen.queryByRole("button", { name: "Stop" })).toBeNull();
});

it("always shows Delete button", async () => {
  mockFetch({ "/api/v1/instances": MIXED });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  const deleteButtons = screen.getAllByRole("button", { name: "Delete" });
  expect(deleteButtons).toHaveLength(2);
});

// ── create dialog ─────────────────────────────────────────────────────────────

it("opens create dialog on + Create click", async () => {
  mockFetch({ "/api/v1/instances": [] });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText(/no containers found/i));
  fireEvent.click(screen.getByRole("button", { name: /create/i }));
  expect(screen.getByText("Create Container")).toBeInTheDocument();
});

it("shows validation error when name is empty", async () => {
  mockFetch({ "/api/v1/instances": [] });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText(/no containers found/i));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));
  // Click Create inside the dialog without filling in a name
  const dialog = screen.getByText("Create Container").closest("div")!.parentElement!;
  fireEvent.click(within(dialog).getByRole("button", { name: /^create$/i }));
  expect(screen.getByText(/name is required/i)).toBeInTheDocument();
});

it("closes create dialog on Cancel", async () => {
  mockFetch({ "/api/v1/instances": [] });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText(/no containers found/i));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));
  fireEvent.click(screen.getByRole("button", { name: /cancel/i }));
  expect(screen.queryByText("Create Container")).toBeNull();
});

// ── delete confirm dialog ─────────────────────────────────────────────────────

it("opens confirm dialog on Delete click", async () => {
  mockFetch({ "/api/v1/instances": RUNNING });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  // Multiple "Delete" buttons may exist (one per row); click the first.
  fireEvent.click(screen.getAllByRole("button", { name: "Delete" })[0]);
  expect(screen.getByText(/delete container/i)).toBeInTheDocument();
  // "web-01" now appears in both the table row and the dialog; use getAllBy.
  expect(screen.getAllByText(/web-01/).length).toBeGreaterThan(0);
});

it("closes confirm dialog on Cancel", async () => {
  mockFetch({ "/api/v1/instances": RUNNING });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  fireEvent.click(screen.getAllByRole("button", { name: "Delete" })[0]);
  fireEvent.click(screen.getByRole("button", { name: "Cancel" }));
  expect(screen.queryByText(/delete container/i)).toBeNull();
});

it("calls DELETE endpoint on confirm", async () => {
  mockFetch({ "/api/v1/instances": RUNNING });
  render(<ContainersPage />);
  await waitFor(() => screen.getByText("web-01"));
  fireEvent.click(screen.getAllByRole("button", { name: "Delete" })[0]);
  // Click the red "Delete" button inside the ConfirmDialog
  const confirmBtn = screen.getAllByRole("button", { name: "Delete" }).at(-1)!;
  fireEvent.click(confirmBtn);
  await waitFor(() => {
    const calls = (global.fetch as ReturnType<typeof vi.fn>).mock.calls;
    const deleteCall = calls.find(([url, opts]: [string, RequestInit]) =>
      url.includes("web-01") && opts?.method === "DELETE"
    );
    expect(deleteCall).toBeDefined();
  });
});
