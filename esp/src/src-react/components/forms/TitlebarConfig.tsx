import * as React from "react";
import { Checkbox, ColorPicker, DefaultButton, getColorFromString, IColor, Label, PrimaryButton, TextField } from "@fluentui/react";
import { useForm, Controller } from "react-hook-form";

import nlsHPCC from "src/nlsHPCC";
import { MessageBox } from "../../layouts/MessageBox";

interface TitlebarConfigValues {
    showEnvironmentTitle: boolean;
    environmentTitle: string;
    titlebarColor: string;
}

const defaultValues: TitlebarConfigValues = {
    showEnvironmentTitle: false,
    environmentTitle: "",
    titlebarColor: ""
};

interface TitlebarConfigProps {
    showEnvironmentTitle: string;
    setShowEnvironmentTitle: (_: string) => void;
    environmentTitle: string;
    setEnvironmentTitle: (_: string) => void;
    titlebarColor: string;
    setTitlebarColor: (_: string) => void;
    showForm: boolean;
    setShowForm: (_: boolean) => void;
}

const white = getColorFromString("#ffffff");

export const TitlebarConfig: React.FunctionComponent<TitlebarConfigProps> = ({
    showEnvironmentTitle,
    setShowEnvironmentTitle,
    environmentTitle,
    setEnvironmentTitle,
    titlebarColor,
    setTitlebarColor,
    showForm,
    setShowForm
}) => {
    const { handleSubmit, control, reset } = useForm<TitlebarConfigValues>({ defaultValues });
    const [color, setColor] = React.useState(white);
    const updateColor = React.useCallback((evt: any, colorObj: IColor) => setColor(colorObj), []);

    const closeForm = React.useCallback(() => {
        setShowForm(false);
    }, [setShowForm]);

    const onSubmit = React.useCallback(() => {
        handleSubmit(
            (data, evt) => {
                const request: any = data;
                request.titlebarColor = color.str;

                setShowEnvironmentTitle(request?.showEnvironmentTitle.toString());
                setEnvironmentTitle(request?.environmentTitle);
                setTitlebarColor(request.titlebarColor);

                closeForm();
            },
        )();
    }, [closeForm, color, handleSubmit, setEnvironmentTitle, setShowEnvironmentTitle, setTitlebarColor]);

    React.useEffect(() => {
        setColor(getColorFromString(titlebarColor));
        const values = {
            showEnvironmentTitle: showEnvironmentTitle === "false" ? false : true,
            environmentTitle: environmentTitle || ""
        };
        reset(values);
    }, [environmentTitle, reset, showEnvironmentTitle, titlebarColor]);

    return <MessageBox show={showForm} setShow={closeForm} title={nlsHPCC.SetToolbarColor} minWidth={400}
        footer={<>
            <PrimaryButton text={nlsHPCC.OK} onClick={handleSubmit(onSubmit)} />
            <DefaultButton text={nlsHPCC.Cancel} onClick={() => { reset(defaultValues); closeForm(); }} />
        </>}>
        <Controller
            control={control} name="showEnvironmentTitle"
            render={({
                field: { onChange, name: fieldName, value },
                fieldState: { error }
            }) => <Controller
                    control={control} name={fieldName}
                    render={({
                        field: { onChange, name: fieldName, value }
                    }) => <Checkbox name={fieldName} checked={value} onChange={onChange} label={nlsHPCC.EnableBannerText} />}
                />
            }
        />
        <Controller
            control={control} name="environmentTitle"
            render={({
                field: { onChange, name: fieldName, value },
                fieldState: { error }
            }) => <TextField
                    name={fieldName}
                    onChange={onChange}
                    required={true}
                    label={nlsHPCC.NameOfEnvironment}
                    value={value}
                    errorMessage={error && error?.message}
                />}
            rules={{
                required: nlsHPCC.ValidationErrorRequired
            }}
        />
        <Label>{nlsHPCC.BannerColor}</Label>
        <ColorPicker
            onChange={updateColor}
            color={color}
        />
    </MessageBox>;

};