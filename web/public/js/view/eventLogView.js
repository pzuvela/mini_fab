export function renderEventLog(events, elements) {
    const fragment = document.createDocumentFragment();

    for (const event of events) {
        const row = document.createElement("tr");

        const values = [
            event.sequenceId,
            new Date(event.timestamp).toLocaleTimeString(),
            event.type,
            event.severity,
            event.message
        ];

        values.forEach((value, index) => {
            const cell = document.createElement("td");
            cell.textContent = value ?? "";

            if (index === 3) {
                cell.className =
                    `severity severity-${String(value).toLowerCase()}`;
            }

            row.appendChild(cell);
        });

        fragment.appendChild(row);
    }

    elements.eventRows.replaceChildren(fragment);
    elements.eventCount.textContent = `${events.length} events`;
}

