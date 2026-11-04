const MiniFabConfig = Object.freeze({
    version: "0.3.0",
    name: "MiniFab"
});

document.querySelectorAll("[data-app-version]")
    .forEach(element => {
        element.textContent = `SIMULATOR · v${MiniFabConfig.version}`;
    });
