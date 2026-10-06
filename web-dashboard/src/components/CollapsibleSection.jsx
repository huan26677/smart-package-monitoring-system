import { useId, useState } from "react";

const ICONS = {
  overview: "M3 3h7v7H3z M14 3h7v7h-7z M3 14h7v7H3z M14 14h7v7h-7z",
  notices: "M12 3 2 21h20L12 3z M12 9v5 M12 17h.01",
  sensors: "M3 18V6 M3 18h18 M6 14l4-5 4 3 6-7",
  events: "M8 4H5v17h14V4h-3 M8 3h8v4H8z M8 12h8 M8 16h5",
  ai: "M12 8a4 4 0 1 0 0 8 4 4 0 0 0 0-8z M12 3v3 M12 18v3 M3 12h3 M18 12h3 M5.6 5.6l2.1 2.1 M16.3 16.3l2.1 2.1 M5.6 18.4l2.1-2.1 M16.3 7.7l2.1-2.1",
  map: "M12 21s7-6 7-12a7 7 0 0 0-14 0c0 6 7 12 7 12z M12 6a3 3 0 1 0 0 6 3 3 0 0 0 0-6z",
  locations: "M5 5a2 2 0 1 0 0 .01 M19 19a2 2 0 1 0 0 .01 M7 5h8a4 4 0 0 1 0 8H9a3 3 0 0 0 0 6h8",
  wifi: "M2 8a16 16 0 0 1 20 0 M5 12a11 11 0 0 1 14 0 M8 16a6 6 0 0 1 8 0 M12 20h.01",
};

export default function CollapsibleSection({ id, title, description, badge, tone = "neutral", defaultOpen = false, eager = false, children }) {
  const storageKey = `dashboard-section:${id}`;
  const [open, setOpen] = useState(() => {
    try {
      const saved = localStorage.getItem(storageKey);
      if (saved === "open" || saved === "closed") return saved === "open";
    } catch { /* Layout also works without browser storage. */ }
    return defaultOpen;
  });
  const [visited, setVisited] = useState(() => open || eager);
  const regionId = useId();
  const titleId = useId();

  function toggle() {
    const next = !open;
    setOpen(next);
    if (next) setVisited(true);
    try { localStorage.setItem(storageKey, next ? "open" : "closed"); } catch { /* Keep the current layout in memory. */ }
  }

  return <section className={`function-section ${open ? "is-open" : "is-closed"}`} data-section={id}>
    <h2 className="function-heading">
      <button type="button" className="function-toggle" aria-label={title}
        aria-expanded={open} aria-controls={regionId} onClick={toggle}>
        <span className={`function-icon icon-${id}`} aria-hidden="true">
          <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.7" strokeLinecap="round" strokeLinejoin="round"><path d={ICONS[id] || ICONS.overview}/></svg>
        </span>
        <span className="function-label"><span id={titleId} className="function-title">{title}</span>
          {description && <span className="function-description">{description}</span>}
        </span>
        {badge && <span className={`function-badge badge-${tone}`}>{badge}</span>}
        <span className="function-action" aria-hidden="true">{open ? "Thu gọn" : "Mở"}</span>
        <svg className="function-chevron" aria-hidden="true" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round"><path d="m6 9 6 6 6-6"/></svg>
      </button>
    </h2>
    <div id={regionId} className="function-body" role="region" aria-labelledby={titleId} hidden={!open}>
      {visited && children}
    </div>
  </section>;
}
