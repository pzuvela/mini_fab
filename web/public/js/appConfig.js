const MiniFabConfig = Object.freeze({
    version: window.MINIFAB_VERSION,
    name: "MiniFab"
});

document.querySelectorAll("[data-app-version]")
    .forEach(element => {
        element.textContent =
            `SIMULATOR · v${MiniFabConfig.version}`;
    });

