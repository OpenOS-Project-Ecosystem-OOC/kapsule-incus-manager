import { render, screen, fireEvent, waitFor } from "@testing-library/react";
import { NetworksPage } from "../pages/NetworksPage";

const NETWORKS = [
  { name: "lxdbr0", type: "bridge",   managed: true,  description: "Default bridge" },
  { name: "eth0",   type: "physical", managed: false, description: "" },
];

function mockFetch(data: unknown = NETWORKS) {
  vi.spyOn(global, "fetch").mockImplementation(async () =>
    new Response(JSON.stringify(data), {
      status: 200,
      headers: { "Content-Type": "application/json" },
    })
  );
}

afterEach(() => vi.restoreAllMocks());

// ── rendering ─────────────────────────────────────────────────────────────────

it("renders network rows", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  expect(screen.getByText("eth0")).toBeInTheDocument();
  expect(screen.getByText("bridge")).toBeInTheDocument();
  expect(screen.getByText("physical")).toBeInTheDocument();
});

it("shows empty state when no networks", async () => {
  mockFetch([]);
  render(<NetworksPage />);
  await waitFor(() => expect(screen.getByText(/no networks found/i)).toBeInTheDocument());
});

it("shows error on fetch failure", async () => {
  vi.spyOn(global, "fetch").mockRejectedValue(new Error("Daemon unreachable"));
  render(<NetworksPage />);
  await waitFor(() => expect(screen.getByText(/daemon unreachable/i)).toBeInTheDocument());
});

it("shows loading state initially", () => {
  mockFetch();
  render(<NetworksPage />);
  expect(screen.getByText(/loading/i)).toBeInTheDocument();
});

// ── create dialog ─────────────────────────────────────────────────────────────

it("opens create dialog on + Create click", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));
  expect(screen.getByText("Create Network")).toBeInTheDocument();
});

it("shows validation error when name is empty", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));
  fireEvent.click(screen.getByRole("button", { name: /^create$/i }));
  expect(screen.getByText(/name is required/i)).toBeInTheDocument();
});

it("closes create dialog on Cancel", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));
  fireEvent.click(screen.getByRole("button", { name: /cancel/i }));
  expect(screen.queryByText("Create Network")).toBeNull();
});

it("calls POST networks on valid create", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getByRole("button", { name: /\+ create/i }));

  const nameInput = screen.getByRole("textbox");
  fireEvent.change(nameInput, { target: { value: "mybr0" } });
  fireEvent.click(screen.getByRole("button", { name: /^create$/i }));

  await waitFor(() => {
    const calls = (global.fetch as ReturnType<typeof vi.fn>).mock.calls;
    const postCall = calls.find(([url, opts]: [string, RequestInit]) =>
      url.includes("/api/v1/networks") && opts?.method === "POST"
    );
    expect(postCall).toBeDefined();
    const body = JSON.parse(postCall![1]?.body as string);
    expect(body.name).toBe("mybr0");
  });
});

// ── delete ────────────────────────────────────────────────────────────────────

it("shows Delete button only for managed networks", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  // lxdbr0 is managed=true, eth0 is managed=false → only 1 Delete button
  const deleteButtons = screen.getAllByRole("button", { name: /delete/i });
  expect(deleteButtons).toHaveLength(1);
});

it("opens confirm dialog on Delete click", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getAllByRole("button", { name: /delete/i })[0]);
  expect(screen.getByText(/delete network/i)).toBeInTheDocument();
});

it("calls DELETE endpoint on confirm", async () => {
  mockFetch();
  render(<NetworksPage />);
  await waitFor(() => screen.getByText("lxdbr0"));
  fireEvent.click(screen.getAllByRole("button", { name: /delete/i })[0]);
  // Two "Delete" buttons exist: the row trigger and the confirm dialog button.
  // Pick the last one (the confirm dialog's confirm button).
  const deleteButtons = screen.getAllByRole("button", { name: /^delete$/i });
  fireEvent.click(deleteButtons[deleteButtons.length - 1]);
  await waitFor(() => {
    const calls = (global.fetch as ReturnType<typeof vi.fn>).mock.calls;
    const deleteCall = calls.find(([url, opts]: [string, RequestInit]) =>
      url.includes("lxdbr0") && opts?.method === "DELETE"
    );
    expect(deleteCall).toBeDefined();
  });
});
