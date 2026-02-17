export module th.gui;

export namespace th::ui {

class IComponent {
public:
    IComponent() = default;
    IComponent(IComponent const&) = default;
    auto operator=(IComponent const&) -> IComponent& = default;
    IComponent(IComponent&&) = default;
    auto operator=(IComponent&&) -> IComponent& = default;

    virtual void draw() = 0;

    virtual ~IComponent() = default;
};

}// namespace th::ui
