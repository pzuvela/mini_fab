export function bindRunControls(elements, vm) {
    elements.runButton.addEventListener("click", () => {
        void vm.run();
    });

    elements.cancelButton.addEventListener("click", () => {
        void vm.cancelRun();
    });

    elements.resetButton.addEventListener("click", () => {
        void vm.resetEquipment();
    });

    elements.recipeSelect.addEventListener("change", event => {
        const value = event.target.value;
        vm.selectRecipe(value === "" ? null : Number(value));
    });

    elements.waferSelect.addEventListener("change", event => {
        vm.selectWafer(event.target.value || null);
    });
}


